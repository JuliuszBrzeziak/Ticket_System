#include <gtest/gtest.h>
#include "core/TicketPool.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

class TicketPoolTest : public ::testing::Test
{
protected:
    TicketPool pool;

    void SetUp() override
    {
        pool.addTickets("normalny", 350, 10);
        pool.addTickets("ulgowy", 170, 5);
    }
};

TEST_F(TicketPoolTest, ShouldAddAndReadAvailableTickets)
{
    auto available = pool.getAvailableTickets();
    EXPECT_EQ(available["normalny"], 10);
    EXPECT_EQ(available["ulgowy"], 5);
}

TEST_F(TicketPoolTest, ShouldReserveTicketAndDecreaseCount)
{
    auto resId = pool.reserveTicket("normalny");
    ASSERT_TRUE(resId.has_value());

    auto available = pool.getAvailableTickets();
    EXPECT_EQ(available["normalny"], 9);

    auto price = pool.getReservationPrice(resId.value());
    ASSERT_TRUE(price.has_value());
    EXPECT_EQ(price.value(), 350);
}

TEST_F(TicketPoolTest, ShouldCancelReservationAndReturnTicketToPool)
{
    auto resId = pool.reserveTicket("normalny");
    bool canceled = pool.cancelReservation(resId.value());

    EXPECT_TRUE(canceled);
    auto available = pool.getAvailableTickets();
    EXPECT_EQ(available["normalny"], 10);
}

TEST_F(TicketPoolTest, ShouldNotCancelFinalizedReservation)
{
    auto resId = pool.reserveTicket("normalny");
    bool finalized = pool.finalizePurchase(resId.value(), "Jan Kowalski");
    EXPECT_TRUE(finalized);

    bool canceled = pool.cancelReservation(resId.value());
    EXPECT_FALSE(canceled);

    auto available = pool.getAvailableTickets();
    EXPECT_EQ(available["normalny"], 9);
}

TEST_F(TicketPoolTest, ShouldClearExpiredReservations)
{
    auto resId = pool.reserveTicket("normalny");

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    int cleared = pool.clearExpiredReservations(1);
    EXPECT_EQ(cleared, 1);

    auto available = pool.getAvailableTickets();
    EXPECT_EQ(available["normalny"], 10);
}

TEST_F(TicketPoolTest, ShouldHandleConcurrentReservationsSafely)
{
    TicketPool concurrentPool;
    concurrentPool.addTickets("festiwal", 10000, 50);

    std::vector<std::thread> threads;
    std::atomic<int> successfulReservations{0};

    for (int i = 0; i < 100; ++i)
    {
        threads.emplace_back([&]()
                             {
            auto res = concurrentPool.reserveTicket("festiwal");
            if (res.has_value()) {
                successfulReservations++;
            } });
    }

    for (auto &t : threads)
    {
        t.join();
    }

    EXPECT_EQ(successfulReservations.load(), 50);
    auto available = concurrentPool.getAvailableTickets();
    EXPECT_EQ(available["festiwal"], 0);
}