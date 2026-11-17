#pragma once

namespace GameLogic::Items
{
// Lahap accepts only whole smaller bundles and singles, never a split or a target-size bundle.
bool CanCombineJewels(int singleCount, int tenBundles, int twentyBundles, int targetCount);
} // namespace GameLogic::Items
