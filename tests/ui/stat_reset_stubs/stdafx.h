#pragma once
#include <array>
#include <functional>
#include <string>
#include <vector>

inline constexpr int MAX_EQUIPMENT = 16;
struct StatResetTestEquipment
{
    int Type = -1;
};
struct StatResetTestCharacter
{
    std::array<StatResetTestEquipment, MAX_EQUIPMENT> Equipment;
};
inline StatResetTestCharacter statResetTestCharacter;
inline StatResetTestCharacter* CharacterMachine = &statResetTestCharacter;

inline constexpr unsigned long RGBA(unsigned long r, unsigned long g, unsigned long b, unsigned long a)
{
    return r | (g << 8) | (b << 16) | (a << 24);
}
