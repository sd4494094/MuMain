#include "BloodCastleObjective.h"

namespace GameLogic::Events
{
namespace
{
constexpr std::uint8_t kStarted = 0;
constexpr std::uint8_t kGateNotDestroyed = 1;
constexpr std::uint8_t kGateDestroyed = 4;
constexpr std::uint8_t kFirstQuestWeapon = 1;
constexpr std::uint8_t kLastQuestWeapon = 3;
// OpenMU currently sends 0x00FF without an owner; the original protocol uses 0xFFFF.
constexpr std::uint16_t kOpenMuNoOwner = 0x00FF;
constexpr std::uint16_t kOriginalNoOwner = 0xFFFF;
constexpr int kGateTargetCount = 1;
} // namespace

BloodCastleObjective BloodCastleObjective::FromState(std::uint8_t state, int required, int killed,
                                                     std::uint8_t itemType, std::uint16_t itemOwner)
{
    if (state == kStarted || state == kGateNotDestroyed)
    {
        if (required <= 0)
            return {};
        if (required == kGateTargetCount || killed >= required)
            return {Phase::DestroyGate};
        return {Phase::KillMonsters, killed, required};
    }

    if (state != kGateDestroyed)
        return {};

    if (itemType >= kFirstQuestWeapon && itemType <= kLastQuestWeapon)
    {
        const bool hasOwner = itemOwner != kOpenMuNoOwner && itemOwner != kOriginalNoOwner;
        return {hasOwner ? Phase::ReturnWeapon : Phase::PickUpWeapon};
    }

    if (required > 0 && killed < required)
        return {Phase::KillSorcerers, killed, required};

    return {Phase::DestroyStatue};
}
} // namespace GameLogic::Events
