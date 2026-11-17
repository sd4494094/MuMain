#include "JewelMixMaterials.h"

#include <algorithm>

namespace GameLogic::Items
{
bool CanCombineJewels(int singleCount, int tenBundles, int twentyBundles, int targetCount)
{
    constexpr int kTen = 10;
    constexpr int kTwenty = 20;
    constexpr int kThirty = 30;
    if ((targetCount != kTen && targetCount != kTwenty && targetCount != kThirty) || singleCount < 0 ||
        tenBundles < 0 || twentyBundles < 0)
        return false;

    int remaining = targetCount;
    if (targetCount > kTwenty && twentyBundles > 0)
        remaining -= kTwenty;
    if (targetCount > kTen)
        remaining -= std::min(tenBundles, remaining / kTen) * kTen;
    return singleCount >= remaining;
}
} // namespace GameLogic::Items
