#pragma once

#include <cstdint>

namespace GameLogic::Events
{
// A gameplay objective inferred from the existing 0x9B state packet, independent of its presentation.
struct BloodCastleObjective
{
    enum class Phase
    {
        None,
        KillMonsters,
        DestroyGate,
        KillSorcerers,
        DestroyStatue,
        PickUpWeapon,
        ReturnWeapon,
    };

    Phase phase = Phase::None;
    int killed = 0;
    int required = 0;

    static BloodCastleObjective FromState(std::uint8_t state, int required, int killed, std::uint8_t itemType,
                                          std::uint16_t itemOwner);
    bool operator==(const BloodCastleObjective&) const = default;
};
} // namespace GameLogic::Events
