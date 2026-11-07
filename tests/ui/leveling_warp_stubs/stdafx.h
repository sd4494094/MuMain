#pragma once
#include <functional>
#include <string>
#include <vector>
#define MU_C16(x) (x)
inline constexpr int MAIN_SCENE = 5;
inline int SceneFlag = MAIN_SCENE;
struct TestHero
{
    wchar_t ID[16] = L"player";
};
inline TestHero hero;
inline TestHero* Hero = &hero;
