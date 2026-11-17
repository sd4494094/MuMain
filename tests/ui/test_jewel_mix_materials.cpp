#include <doctest.h>

#include "GameLogic/Items/JewelMixMaterials.h"

using GameLogic::Items::CanCombineJewels;

TEST_CASE("jewel combining accepts exact whole materials")
{
    CHECK(CanCombineJewels(30, 0, 0, 30));
    CHECK(CanCombineJewels(0, 2, 0, 20));
    CHECK(CanCombineJewels(0, 3, 0, 30));
    CHECK(CanCombineJewels(0, 1, 1, 30));
    CHECK(CanCombineJewels(10, 1, 0, 20));
    CHECK(CanCombineJewels(10, 0, 1, 30));
    CHECK(CanCombineJewels(20, 1, 0, 30));
    CHECK(CanCombineJewels(10, 2, 0, 30));
    CHECK(CanCombineJewels(0, 4, 2, 30));
}

TEST_CASE("jewel combining never splits or reuses a target-size bundle")
{
    CHECK_FALSE(CanCombineJewels(0, 0, 2, 30));
    CHECK_FALSE(CanCombineJewels(9, 0, 1, 30));
    CHECK_FALSE(CanCombineJewels(0, 1, 0, 10));
    CHECK_FALSE(CanCombineJewels(0, 0, 1, 20));
    CHECK_FALSE(CanCombineJewels(0, 2, 0, 30));
    CHECK_FALSE(CanCombineJewels(100, 10, 10, 0));
    CHECK_FALSE(CanCombineJewels(100, 10, 10, 15));
    CHECK_FALSE(CanCombineJewels(100, 10, 10, 40));
}

TEST_CASE("jewel eligibility matches every exact subset in a bounded inventory")
{
    for (int target : {10, 20, 30})
        for (int singles = 0; singles <= 31; ++singles)
            for (int tens = 0; tens <= 4; ++tens)
                for (int twenties = 0; twenties <= 3; ++twenties)
                {
                    bool exact = false;
                    for (int useTen = 0; useTen <= tens; ++useTen)
                        for (int useTwenty = 0; useTwenty <= twenties; ++useTwenty)
                        {
                            if ((useTen > 0 && target <= 10) || (useTwenty > 0 && target <= 20))
                                continue;
                            const int remaining = target - useTen * 10 - useTwenty * 20;
                            exact |= remaining >= 0 && remaining <= singles;
                        }
                    CAPTURE(target);
                    CAPTURE(singles);
                    CAPTURE(tens);
                    CAPTURE(twenties);
                    CHECK(CanCombineJewels(singles, tens, twenties, target) == exact);
                }
}
