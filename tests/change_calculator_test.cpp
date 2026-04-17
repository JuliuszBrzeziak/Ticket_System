#include <gtest/gtest.h>
#include "core/ChangeCalculator.h"

TEST(ChangeCalculatorTest, ShouldReturnExactSingleCoin)
{
    CoinInventory inventory;
    inventory.addCoin(100, 2);
    inventory.addCoin(50, 1);

    ChangeCalculator calculator;
    auto result = calculator.calculateChange(50, inventory);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->at(50), 1);
}

TEST(ChangeCalculatorTest, ShouldReturnChangeForOneFifty)
{
    CoinInventory inventory;
    inventory.addCoin(200, 1);
    inventory.addCoin(100, 5);
    inventory.addCoin(50, 2);

    ChangeCalculator calculator;
    auto result = calculator.calculateChange(150, inventory);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->at(100), 1);
    EXPECT_EQ(result->at(50), 1);
}

TEST(ChangeCalculatorTest, ShouldFailWhenChangeCannotBeGiven)
{
    CoinInventory inventory;
    inventory.addCoin(200, 10);

    ChangeCalculator calculator;
    auto result = calculator.calculateChange(220, inventory);

    EXPECT_FALSE(result.has_value());
}

TEST(ChangeCalculatorTest, ShouldPreferSmallerNumberOfCoins)
{
    CoinInventory inventory;
    inventory.addCoin(100, 3);
    inventory.addCoin(50, 10);

    ChangeCalculator calculator;
    auto result = calculator.calculateChange(200, inventory);

    ASSERT_TRUE(result.has_value());

    int total_coins = 0;
    for (const auto &[denom, count] : *result)
    {
        total_coins += count;
    }

    EXPECT_EQ(total_coins, 2);
    EXPECT_EQ(result->at(100), 2);
}