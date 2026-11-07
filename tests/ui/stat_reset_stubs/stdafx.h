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
