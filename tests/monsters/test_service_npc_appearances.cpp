#include "stdafx.h"

#include "MonsterCreationScope.h"
#include "MonsterModelRecorder.h"

#include "Core/Globals/_enum.h"
#include "Data/GameData/MonsterData/MonsterModelSlots.h"
#include "Engine/Object/ZzzOpenData.h"
#include "Render/Models/ZzzBMD.h"

#include <doctest.h>
#include <cstdlib>

namespace
{
constexpr int NoahNumber = 710;
constexpr int LeoNumber = 371;
constexpr int NoahSlot = MODEL_MONSTER01 + static_cast<int>(MONSTER_MODEL_SERVICE_NOAH);
constexpr int LeoSlot = MODEL_MONSTER01 + static_cast<int>(MONSTER_MODEL_SERVICE_LEO);

bool HasPrivateAssets()
{
    if (std::getenv("MU_TEST_PRIVATE_SERVICE_NPCS") != nullptr)
        return true;
    MESSAGE("MU_TEST_SKIPPED: private licensed NPC assets are not enabled");
    return false;
}
} // namespace

TEST_CASE("Service NPC appearances: independent slots preserve existing model IDs [service-npc]")
{
    CHECK(MONSTER_MODEL_NACONDRA == 216);
    CHECK(MONSTER_MODEL_SERVICE_NOAH == 217);
    CHECK(MONSTER_MODEL_SERVICE_LEO == 218);
    CHECK(MODEL_MONSTER_END == MODEL_MONSTER01 + MAX_MODEL_MONSTER);
    CHECK(Data::Monsters::FindMonsterModelSlot("MONSTER_MODEL_SERVICE_NOAH") == NoahSlot);
    CHECK(Data::Monsters::FindMonsterModelSlot("MONSTER_MODEL_SERVICE_LEO") == LeoSlot);
    CHECK(Data::Monsters::GetMonsterModelReference(NoahSlot) == "MONSTER_MODEL_SERVICE_NOAH");
    CHECK(Data::Monsters::GetMonsterModelReference(LeoSlot) == "MONSTER_MODEL_SERVICE_LEO");
}

TEST_CASE("Service NPC appearances: actual BMD loaders resolve all private textures and animated bones [service-npc]")
{
    if (!HasPrivateAssets())
        return;
    MonsterRecorder::LoadScope scope(0);
    const auto changes = scope.Run(
        []
        {
            OpenMonsterModel(MONSTER_MODEL_SERVICE_NOAH);
            OpenMonsterModel(MONSTER_MODEL_SERVICE_LEO);
        });
    REQUIRE(changes.textureErrors.empty());
    for (const int slot : {NoahSlot, LeoSlot})
    {
        const auto loaded = scope.ReadSlot(slot);
        CHECK(loaded.numMeshs == (slot == NoahSlot ? 4 : 10));
        CHECK(loaded.numBones == (slot == NoahSlot ? 61 : 60));
        CHECK(loaded.numActions == (slot == NoahSlot ? 2 : 3));
        CHECK(loaded.boneHead == (slot == NoahSlot ? 7 : 20));
        for (const auto& action : loaded.actions)
        {
            CHECK(action.keys > 1);
            CHECK(action.speed > 0.f);
        }
        for (const auto& texture : loaded.textures)
        {
            CHECK(texture.bitmap != L"unknown");
            CHECK(texture.bitmap != L"hidden");
        }
    }
}

TEST_CASE("Service NPC appearances: creation keeps NPC kind bounds and unrelated helpers [service-npc]")
{
    if (!HasPrivateAssets())
        return;
    MonsterRecorder::CreationScope scope(0);
    for (const int number : {NoahNumber, LeoNumber})
    {
        CHARACTER* character = scope.Create(number);
        REQUIRE(character != nullptr);
        CHECK(character->Object.Type == (number == NoahNumber ? NoahSlot : LeoSlot));
        CHECK(character->Object.Kind == KIND_NPC);
        CHECK(character->MonsterIndex == number);
        CHECK(character->Object.BoundingBoxMax[2] == (number == NoahNumber ? 190.f : 205.f));
        CHECK(character->Object.CurrentAction < Models[character->Object.Type].NumActions);
        CHECK(character->Weapon[0].Type == -1);
        CHECK(character->Weapon[1].Type == -1);
        CHECK(character->BodyPart[BODYPART_ARMOR].Type == -1);
    }
    for (const int number : {static_cast<int>(MONSTER_LUKE_THE_HELPER), static_cast<int>(MONSTER_HELPER_ELLEN)})
    {
        CHARACTER* character = scope.Create(number);
        REQUIRE(character != nullptr);
        CHECK(character->Object.Type == MODEL_PLAYER);
        CHECK(character->BodyPart[BODYPART_ARMOR].Type == MODEL_PLATE_ARMOR);
    }
}
