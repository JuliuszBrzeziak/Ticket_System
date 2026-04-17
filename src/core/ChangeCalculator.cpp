#include "core/ChangeCalculator.h"
#include <vector>
#include <algorithm>

void CoinInventory::addCoin(int denomination, int count)
{
    coins_[denomination] += count;
}

int CoinInventory::getCount(int denomination) const
{
    auto it = coins_.find(denomination);
    if (it == coins_.end())
    {
        return 0;
    }
    return it->second;
}

const std::map<int, int> &CoinInventory::getCoins() const
{
    return coins_;
}

// Pomocnicza funkcja rekurencyjna do szukania reszty
bool ChangeCalculator::findChange(
    int amountLeft,
    const std::vector<std::pair<int, int>> &availableCoins,
    size_t coinIndex,
    std::map<int, int> &currentChange,
    std::map<int, int> &bestChange,
    int &minCoinsUsed,
    int currentCoinsUsed) const
{
    // Sukces - znaleźliśmy sposób na wydanie reszty
    if (amountLeft == 0)
    {
        if (currentCoinsUsed < minCoinsUsed)
        {
            minCoinsUsed = currentCoinsUsed;
            bestChange = currentChange;
        }
        return true;
    }

    // Porażka - zabrakło nam nominałów do sprawdzenia
    if (coinIndex >= availableCoins.size())
    {
        return false;
    }

    bool foundAny = false;
    int denom = availableCoins[coinIndex].first;
    int maxAvailable = availableCoins[coinIndex].second;

    // Ile maksymalnie takich monet możemy wziąć?
    int maxToTake = std::min(maxAvailable, amountLeft / denom);

    // Sprawdzamy opcje od największej możliwej liczby monet do 0
    for (int i = maxToTake; i >= 0; --i)
    {
        // Optymalizacja: jeśli już wzięliśmy więcej monet niż w najlepszym
        // dotychczasowym rozwiązaniu, przerywamy tę ścieżkę
        if (currentCoinsUsed + i >= minCoinsUsed)
        {
            continue;
        }

        if (i > 0)
        {
            currentChange[denom] = i;
        }

        bool result = findChange(
            amountLeft - (i * denom),
            availableCoins,
            coinIndex + 1,
            currentChange,
            bestChange,
            minCoinsUsed,
            currentCoinsUsed + i);

        if (result)
        {
            foundAny = true;
        }

        if (i > 0)
        {
            currentChange.erase(denom); // Backtracking - cofamy zmianę
        }
    }

    return foundAny;
}

std::optional<std::map<int, int>> ChangeCalculator::calculateChange(
    int amount,
    const CoinInventory &inventory) const
{
    if (amount == 0)
    {
        return std::map<int, int>{};
    }

    // Przerzucamy dostępne monety do wektora i sortujemy malejąco
    // żeby algorytm najpierw próbował używać największych nominałów
    std::vector<std::pair<int, int>> availableCoins;
    for (const auto &[denom, count] : inventory.getCoins())
    {
        if (count > 0)
        {
            availableCoins.push_back({denom, count});
        }
    }
    std::sort(availableCoins.begin(), availableCoins.end(),
              [](const auto &a, const auto &b)
              { return a.first > b.first; });

    std::map<int, int> currentChange;
    std::map<int, int> bestChange;
    int minCoinsUsed = 1e9; // Nieskończoność na start

    bool success = findChange(amount, availableCoins, 0, currentChange, bestChange, minCoinsUsed, 0);

    if (success)
    {
        return bestChange;
    }

    return std::nullopt;
}