#include <httplib.h>
#include <nlohmann/json.hpp>
#include "core/TicketPool.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <exception>
#include <filesystem> // Nowy naglowek

using json = nlohmann::json;
namespace fs = std::filesystem;

// --- Helper do wczytywania konfiguracji ---
json loadConfig(const fs::path &filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        throw std::runtime_error("Nie mozna otworzyc pliku " + filepath.string());
    }
    json config;
    file >> config;
    return config;
}

int main(int argc, char *argv[])
{ // Zmiana na argumenty wejściowe
    json config;
    try
    {
        // Z argv[0] bierzemy sciezke do pliku .exe
        fs::path exePath = fs::absolute(fs::path(argv[0]));
        // Cofamy sie o dwa kroki:
        // exePath.parent_path() to katalog build/
        // .parent_path() jeszcze raz to glowny katalog projektu
        fs::path projectRoot = exePath.parent_path().parent_path();

        fs::path configPath = projectRoot / "config.json";

        std::cout << "[Serwer] Szukam configu w: " << configPath << "\n";
        config = loadConfig(configPath);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[BLAD] " << e.what() << "\n";
        return 1;
    }

    TicketPool pool;
    // ... reszta kodu zostaje bez zmian ...

    // 1. Inicjalizacja biletow z JSON
    for (const auto &ticket : config["tickets"])
    {
        std::string name = ticket["name"];
        int price = ticket["price_in_cents"];
        int count = ticket["initial_count"];
        pool.addTickets(name, price, count);
        std::cout << "[Serwer] Zaladowano bilet: " << name << " (" << count << " szt.)\n";
    }

    int port = config["server"]["port"].get<int>();
    int timeoutSeconds = config["server"]["timeout_seconds"].get<int>();

    httplib::Server svr;

    // 2. Endpoint: Pobranie dostepnych biletow
    svr.Get("/tickets", [&](const httplib::Request & /*req*/, httplib::Response &res)
            {
        json j = pool.getAvailableTickets(); 
        res.set_content(j.dump(), "application/json"); });

    // 3. Endpoint: Rezerwacja biletu
    svr.Post("/reserve", [&](const httplib::Request &req, httplib::Response &res)
             {
        try {
            auto body = json::parse(req.body);
            std::string ticketName = body.at("ticket_name");
            
            auto resId = pool.reserveTicket(ticketName);
            if (resId.has_value()) {
                auto price = pool.getReservationPrice(resId.value());
                json j = {
                    {"reservation_id", resId.value()},
                    {"price", price.value()}
                };
                res.set_content(j.dump(), "application/json");
            } else {
                res.status = 400;
                res.set_content(R"({"error": "Bilet niedostepny"})", "application/json");
            }
        } catch (...) {
            res.status = 400;
        } });

    // 4. Endpoint: Anulowanie rezerwacji
    svr.Post("/cancel", [&](const httplib::Request &req, httplib::Response &res)
             {
        try {
            auto body = json::parse(req.body);
            std::string resId = body.at("reservation_id");
            
            if (pool.cancelReservation(resId)) {
                res.set_content(R"({"status": "ok"})", "application/json");
            } else {
                res.status = 400;
            }
        } catch (...) {
            res.status = 400;
        } });

    // 5. Endpoint: Finalizacja zakupu
    svr.Post("/finalize", [&](const httplib::Request &req, httplib::Response &res)
             {
        try {
            auto body = json::parse(req.body);
            std::string resId = body.at("reservation_id");
            std::string owner = body.at("owner_name");
            
            if (pool.finalizePurchase(resId, owner)) {
                res.set_content(R"({"status": "ok"})", "application/json");
            } else {
                res.status = 400;
            }
        } catch (...) {
            res.status = 400;
        } });

    // 6. Watek czyszczacy (Timeout)
    std::atomic<bool> running{true};
    std::thread cleanupThread([&]()
                              {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            int cleared = pool.clearExpiredReservations(timeoutSeconds);
            if (cleared > 0) {
                std::cout << "[Serwer] Timeout! Zwolniono " << cleared << " rezerwacji.\n";
            }
        } });

    std::cout << "Serwer uruchomiony na porcie " << port << "...\n";
    svr.listen("0.0.0.0", port);

    running = false;
    cleanupThread.join();
    return 0;
}