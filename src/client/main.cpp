#include <httplib.h>
#include <nlohmann/json.hpp>
#include "core/ChangeCalculator.h" // Podpinamy Twoj system wydawania reszty!

#include <iostream>
#include <iomanip>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <fstream>
#include <filesystem> // Nowy naglowek

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace
{
    // Globalny stan kasy automatu - na start pusty
    CoinInventory machineInventory;

    json loadConfig(const fs::path &filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
            return json::object();
        json config;
        file >> config;
        return config;
    }

    // --- HELPERY DO WALUTY ---
    std::string formatCurrency(int cents)
    {
        int zloty = cents / 100;
        int grosze = cents % 100;

        std::string groszeStr = std::to_string(grosze);
        if (grosze < 10)
        {
            groszeStr = "0" + groszeStr;
        }
        return std::to_string(zloty) + "." + groszeStr + " zl";
    }

    // --- HELPERY HTTP (bez zmian) ---
    void clearInput()
    {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    std::optional<json> getJson(httplib::Client &client, const std::string &path)
    {
        auto res = client.Get(path.c_str());
        if (!res || res->status != 200)
            return std::nullopt;
        try
        {
            return json::parse(res->body);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    std::optional<json> postJson(httplib::Client &client, const std::string &path, const json &body)
    {
        auto res = client.Post(path.c_str(), body.dump(), "application/json");
        if (!res)
            return std::nullopt;
        try
        {
            return json{{"status", res->status}, {"body", json::parse(res->body)}};
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    // --- AKCJE BILETOMATU ---
    void printTickets(const json &tickets)
    {
        std::cout << "\nDostepne bilety:\n";
        if (tickets.empty())
        {
            std::cout << "Brak biletow.\n";
            return;
        }

        int index = 1;
        for (const auto &[name, count] : tickets.items())
        {
            std::cout << index << ". " << name << " (dostepnych: " << count.get<int>() << ")\n";
            index++;
        }
    }

    struct ReservationResult
    {
        std::string reservationId;
        int priceInCents;
    };

    std::optional<ReservationResult> reserveTicket(httplib::Client &client)
    {
        auto tickets = getJson(client, "/tickets");
        if (!tickets || tickets->empty())
        {
            std::cout << "Brak dostepnych biletow.\n";
            return std::nullopt;
        }

        printTickets(*tickets);

        std::cout << "\nPodaj numer biletu do rezerwacji (lub 0 aby anulowac): ";
        int choice = -1;
        if (!(std::cin >> choice) || choice == 0)
        {
            clearInput();
            return std::nullopt;
        }
        clearInput();

        std::string selectedTicketName = "";
        int currentIndex = 1;
        for (const auto &[name, count] : tickets->items())
        {
            if (currentIndex == choice)
            {
                selectedTicketName = name;
                break;
            }
            currentIndex++;
        }

        if (selectedTicketName.empty())
        {
            std::cout << "Niepoprawny numer biletu.\n";
            return std::nullopt;
        }

        auto result = postJson(client, "/reserve", {{"ticket_name", selectedTicketName}});
        if (!result || (*result)["status"].get<int>() != 200)
        {
            std::cout << "Nie udalo sie zarezerwowac biletu.\n";
            return std::nullopt;
        }

        const json &responseBody = (*result)["body"];
        ReservationResult res;
        res.reservationId = responseBody["reservation_id"].get<std::string>();
        res.priceInCents = responseBody["price"].get<int>();

        std::cout << "Rezerwacja utworzona. Wybrano: " << selectedTicketName << "\n";
        std::cout << "Do zaplaty: " << formatCurrency(res.priceInCents) << "\n";

        return res;
    }

    void cancelReservation(httplib::Client &client, const std::string &reservationId)
    {
        auto result = postJson(client, "/cancel", {{"reservation_id", reservationId}});
        if (result && (*result)["status"].get<int>() == 200)
        {
            std::cout << "Rezerwacja anulowana pomyslnie.\n";
        }
    }

    bool processPayment(int priceInCents)
    {
        int insertedTotal = 0;
        std::map<int, int> insertedCoins;

        std::cout << "\n--- EKRAN PLATNOSCI ---\n";
        std::cout << "Do zaplaty pozostalo: " << formatCurrency(priceInCents) << "\n";
        std::cout << "Obslugiwane nominaly to grosze: 10, 20, 50, 100(1zl), 200(2zl), 500(5zl).\n";

        // Petla wrzucania monet
        while (insertedTotal < priceInCents)
        {
            std::cout << "Wrzuc monete (lub wpisz 0 zeby anulowac): ";
            int coin = -1;
            if (!(std::cin >> coin))
            {
                clearInput();
                std::cout << "Nierozpoznana moneta.\n";
                continue;
            }
            clearInput();

            if (coin == 0)
            {
                std::cout << "Przerwano platnosc. Zwracam wpłacone " << formatCurrency(insertedTotal) << "...\n";
                return false;
            }

            if (coin != 10 && coin != 20 && coin != 50 && coin != 100 && coin != 200 && coin != 500)
            {
                std::cout << "Automat nie przyjmuje takiej monety.\n";
                continue;
            }

            insertedTotal += coin;
            insertedCoins[coin]++;

            if (insertedTotal < priceInCents)
            {
                std::cout << "Wrzucono " << formatCurrency(insertedTotal)
                          << ". Brakuje: " << formatCurrency(priceInCents - insertedTotal) << "\n";
            }
        }

        std::cout << "Wrzucono lacznie: " << formatCurrency(insertedTotal) << "\n";

        int changeNeeded = insertedTotal - priceInCents;
        if (changeNeeded == 0)
        {
            std::cout << "Zaplata odliczona idealnie. Brak reszty.\n";
            // Dodajemy wrzucone monety na stałe do biletomatu
            for (auto const &[coin, count] : insertedCoins)
            {
                machineInventory.addCoin(coin, count);
            }
            return true;
        }

        // Płatność z resztą - musimy dodać wrzucone monety do maszyny,
        // żeby algorytm mógł ich użyć do ewentualnego wydania reszty.
        for (auto const &[coin, count] : insertedCoins)
        {
            machineInventory.addCoin(coin, count);
        }

        std::cout << "Obliczam reszte: " << formatCurrency(changeNeeded) << "...\n";
        ChangeCalculator calc;
        auto changeResult = calc.calculateChange(changeNeeded, machineInventory);

        if (!changeResult.has_value())
        {
            std::cout << "UWAGA: Biletomat nie ma drobnych do wydania reszty!\n";
            std::cout << "Anulowanie transakcji. Zwracam wrzucone " << formatCurrency(insertedTotal) << "...\n";

            // Ponieważ dorzuciliśmy monety do maszyny chwilę temu, musimy to "cofnąć"
            // (w prawdziwym życiu monety spadają do tacki zwrotu).
            // Zakładam, że Twoje CoinInventory.h posiada metodę getCount/addCoin.
            // Odejmujemy wrzucone monety:
            for (auto const &[coin, count] : insertedCoins)
            {
                // To wywoła addCoin z minusem lub masz osobną metodę removeCoin.
                machineInventory.addCoin(coin, -count);
            }
            return false;
        }

        // Sukces! Wydajemy resztę.
        std::cout << "Wydaje reszte:\n";
        for (const auto &[coin, count] : changeResult.value())
        {
            std::cout << "- " << count << "x " << formatCurrency(coin) << "\n";
        }

        return true;
    }

    bool finalizePurchase(httplib::Client &client, const std::string &reservationId)
    {
        std::cout << "Podaj imie i nazwisko (do biletu): ";
        std::string ownerName;
        std::getline(std::cin, ownerName);

        if (ownerName.empty())
            ownerName = "Anonim";

        auto result = postJson(client, "/finalize", {{"reservation_id", reservationId}, {"owner_name", ownerName}});

        if (result && (*result)["status"].get<int>() == 200)
        {
            std::cout << "\n>>> ZAKUP ZAKONCZONY POWODZENIEM! Drukuję bilet dla: " << ownerName << " <<<\n";
            return true;
        }

        std::cout << "Finalizacja nieudana (byc moze minal czas rezerwacji).\n";
        return false;
    }
}

int main(int argc, char *argv[]) // Zmiana na argumenty wejściowe
{
    fs::path exePath = fs::absolute(fs::path(argv[0]));
    fs::path projectRoot = exePath.parent_path().parent_path();
    fs::path configPath = projectRoot / "config.json";

    json config = loadConfig(configPath);
    // Inicjalizacja monet startowych
    if (config.contains("initial_coins"))
    {
        for (auto &[coinStr, count] : config["initial_coins"].items())
        {
            machineInventory.addCoin(std::stoi(coinStr), count.get<int>());
        }
    }

    // Bezpieczne wczytanie portu
    int port = 8080; // domyślnie
    if (config.contains("server") && config["server"].contains("port"))
    {
        port = config["server"]["port"].get<int>();
    }

    httplib::Client client("localhost", port);
    client.set_connection_timeout(5);
    client.set_read_timeout(5);

    while (true)
    {
        std::cout << "\n=== BILETOMAT ===\n";
        std::cout << "1. Pokaz dostepne bilety\n";
        std::cout << "2. Kup bilet\n";
        std::cout << "0. Wyjscie\n";
        std::cout << "Wybor: ";

        int choice = -1;
        if (!(std::cin >> choice))
        {
            clearInput();
            continue;
        }
        clearInput();

        if (choice == 0)
            break;

        if (choice == 1)
        {
            auto tickets = getJson(client, "/tickets");
            if (tickets)
                printTickets(*tickets);
            continue;
        }

        if (choice == 2)
        {
            auto res = reserveTicket(client);
            if (!res)
                continue;

            // Proces płatności wykorzystujący Etap 2
            bool paymentSuccess = processPayment(res->priceInCents);

            if (!paymentSuccess)
            {
                cancelReservation(client, res->reservationId);
                continue;
            }

            // Jeśli zapłacono, to finalizujemy bilet na serwerze
            if (!finalizePurchase(client, res->reservationId))
            {
                // Skoro zapłacił, ale finalizacja padła (np. timeout serwera),
                // w prawdziwym świecie musielibyśmy zwrócić pieniądze.
                std::cout << "Transakcja po czasie! Zwracam pieniadze.\n";
            }

            continue;
        }
    }

    std::cout << "Do widzenia.\n";
    return 0;
}