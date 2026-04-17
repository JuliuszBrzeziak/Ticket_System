#pragma once

#include <map>

class CoinInventory
{
public:
    void addCoin(int denomination, int count = 1);
    int getCount(int denomination) const;
    const std::map<int, int> &getCoins() const;

private:
    std::map<int, int> coins_;
};