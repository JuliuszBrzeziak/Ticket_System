#include "core/TicketPool.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace
{

    std::string makeReservationId(int counter)
    {
        std::ostringstream oss;
        oss << "RES-" << std::setw(5) << std::setfill('0') << counter;
        return oss.str();
    }
}

void TicketPool::addTickets(const std::string &name, int priceInCents, int count)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ticketTypes_[name] = TicketType{name, priceInCents};
    availableCounts_[name] += count;
}

std::optional<std::string> TicketPool::reserveTicket(const std::string &name)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto countIt = availableCounts_.find(name);
    if (countIt == availableCounts_.end() || countIt->second <= 0)
    {
        return std::nullopt;
    }

    auto typeIt = ticketTypes_.find(name);
    if (typeIt == ticketTypes_.end())
    {
        return std::nullopt;
    }

    countIt->second--;

    const std::string reservationId = makeReservationId(++nextIdCounter_);
    Reservation res;
    res.reservationId = reservationId;
    res.ticketName = name;
    res.priceInCents = typeIt->second.priceInCents;
    res.reservedAt = std::chrono::steady_clock::now();
    res.isFinalized = false;
    res.ownerName = "";

    reservations_[reservationId] = std::move(res);
    return reservationId;
}

bool TicketPool::cancelReservation(const std::string &reservationId)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = reservations_.find(reservationId);
    if (it == reservations_.end())
    {
        return false;
    }

    if (it->second.isFinalized)
    {
        return false;
    }

    availableCounts_[it->second.ticketName]++;
    reservations_.erase(it);
    return true;
}

bool TicketPool::finalizePurchase(const std::string &reservationId,
                                  const std::string &ownerName)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = reservations_.find(reservationId);
    if (it == reservations_.end())
    {
        return false;
    }

    if (it->second.isFinalized)
    {
        return false;
    }

    it->second.isFinalized = true;
    it->second.ownerName = ownerName;
    return true;
}

int TicketPool::clearExpiredReservations(int timeoutSeconds)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const auto now = std::chrono::steady_clock::now();
    int cleared = 0;

    for (auto it = reservations_.begin(); it != reservations_.end(); /* brak ++ */)
    {
        const Reservation &res = it->second;

        if (res.isFinalized)
        {
            ++it;
            continue;
        }

        auto ageSeconds = std::chrono::duration_cast<std::chrono::seconds>(
                              now - res.reservedAt)
                              .count();

        if (ageSeconds >= timeoutSeconds)
        {
            availableCounts_[res.ticketName]++;
            it = reservations_.erase(it);
            ++cleared;
        }
        else
        {
            ++it;
        }
    }

    return cleared;
}

std::map<std::string, int> TicketPool::getAvailableTickets() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return availableCounts_;
}

std::optional<int> TicketPool::getReservationPrice(const std::string &reservationId) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = reservations_.find(reservationId);
    if (it == reservations_.end())
    {
        return std::nullopt;
    }
    return it->second.priceInCents;
}