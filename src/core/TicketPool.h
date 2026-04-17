#pragma once
#include <chrono>
#include <map>
#include <mutex>
#include <optional>
#include <string>

struct TicketType
{
    std::string name;
    int priceInCents;
};

struct Reservation
{
    std::string reservationId;
    std::string ticketName;
    int priceInCents;
    std::chrono::steady_clock::time_point reservedAt;
    bool isFinalized;
    std::string ownerName;
};

class TicketPool
{
public:
    void addTickets(const std::string &name,
                    int priceInCents, int count);
    std::optional<std::string> reserveTicket(const std::string &name);
    bool cancelReservation(const std::string &reservationId);
    bool finalizePurchase(const std::string &reservationId,
                          const std::string &ownerName);
    int clearExpiredReservations(int timeoutSeconds);
    std::map<std::string, int> getAvailableTickets() const;
    std::optional<int> getReservationPrice(const std::string &reservationId) const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, TicketType> ticketTypes_;
    std::map<std::string, int> availableCounts_;
    std::map<std::string, Reservation> reservations_;
    int nextIdCounter_ = 0;
};