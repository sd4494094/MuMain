#pragma once
namespace SEASON3A
{
inline constexpr int MIXTYPE_OSBOURNE = 42;
}
struct RefiningTestRecipe
{
    int type = -1;
    void SetMixType(int value)
    {
        type = value;
    }
};
inline RefiningTestRecipe g_MixRecipeMgr;
