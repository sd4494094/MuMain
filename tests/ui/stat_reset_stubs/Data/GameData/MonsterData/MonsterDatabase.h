#pragma once
struct StatResetTestMonsterDatabase
{
    const wchar_t* name = L"洗点师·艾琳";
    const wchar_t* FindDisplayName(int number)
    {
        return number == 464 ? name : nullptr;
    }
};
inline StatResetTestMonsterDatabase g_MonsterDatabase;
