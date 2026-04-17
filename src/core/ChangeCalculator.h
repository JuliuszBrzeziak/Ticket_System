#pragma once

#include "core/CoinInventory.h"
#include <map>
#include <optional>
#include <vector>

class ChangeCalculator
{
public:
    std::optional<std::map<int, int>> calculateChange(
        int amount,
        const CoinInventory &inventory) const;

private:
    bool findChange(
        int amountLeft,
        const std::vector<std::pair<int, int>> &availableCoins,
        size_t coinIndex,
        std::map<int, int> &currentChange,
        std::map<int, int> &bestChange,
        int &minCoinsUsed,
        int currentCoinsUsed) const;
};