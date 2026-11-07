#include "stdafx.h"

#include "MonsterCreationScope.h"
#include "MonsterModelRecorder.h"

#include "Core/Globals/_enum.h"
#include "Data/GameData/MonsterData/MonsterModelSlots.h"
#include "Engine/Object/ZzzOpenData.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ServiceNpcAppearance.h"
#include "Data/DataHandler/LoadData.h"
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
    for (const int number : {NoahNumber})
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
    for (const int number : {static_cast<int>(MONSTER_LUKE_THE_HELPER)})
    {
        CHARACTER* character = scope.Create(number);
        REQUIRE(character != nullptr);
        CHECK(character->Object.Type == MODEL_PLAYER);
        CHECK(character->BodyPart[BODYPART_ARMOR].Type == MODEL_PLATE_ARMOR);
    }
}

TEST_CASE("Service NPC appearances: Leo uses native +15 excellent Black Dragon parts and Storm wings [service-npc]")
{
    if (!HasPrivateAssets())
        return;
    MonsterRecorder::CreationScope scope(0);
    REQUIRE(gLoadData.AccessModel(MODEL_PLAYER, L"Data\\Player\\", L"player"));
    REQUIRE(Models[MODEL_PLAYER].NumActions > PLAYER_STOP_FLY);
    for (int creation = 0; creation < 2; ++creation)
    {
        CHARACTER* character = scope.Create(LeoNumber);
        REQUIRE(character != nullptr);
        CHECK(character->Object.Type == MODEL_PLAYER);
        CHECK(character->Object.Kind == KIND_NPC);
        CHECK(character->MonsterIndex == LeoNumber);
        CHECK(character->Class == CLASS_BLADEMASTER);
        CHECK(character->NotRotateOnMagicHit);
        const int models[] = {MODEL_BLACK_DRAGON_HELM, MODEL_BLACK_DRAGON_ARMOR, MODEL_BLACK_DRAGON_PANTS,
                              MODEL_BLACK_DRAGON_GLOVES, MODEL_BLACK_DRAGON_BOOTS};
        const int slots[] = {BODYPART_HELM, BODYPART_ARMOR, BODYPART_PANTS, BODYPART_GLOVES, BODYPART_BOOTS};
        for (size_t i = 0; i < std::size(slots); ++i)
        {
            const auto& part = character->BodyPart[slots[i]];
            CHECK(part.Type == models[i]);
            CHECK(part.Level == 15);
            CHECK(part.ExcellentFlags == 0x3F);
            CHECK(part.AncientDiscriminator == 0);
        }
        for (const auto& weapon : character->Weapon)
        {
            CHECK(weapon.Type == MODEL_SWORD_OF_DESTRUCTION);
            CHECK(weapon.Level == 15);
            CHECK(weapon.ExcellentFlags == 0x3F);
        }
        CHECK(character->Wing.Type == MODEL_WING_OF_STORM);
        CHECK(character->Wing.Level == 15);
        CHECK(character->Helper.Type == -1);
        CHECK(character->Object.CurrentAction == PLAYER_STOP_FLY);
        character->SafeZone = true;
        SetPlayerStop(character);
        CHECK(character->Object.CurrentAction == PLAYER_STOP_FLY);
        CHECK(Engine::Object::ServiceNpcs::HasTownHover(character));
        character->Object.Kind = KIND_PLAYER;
        CHECK_FALSE(Engine::Object::ServiceNpcs::HasTownHover(character));
        SetPlayerStop(character);
        CHECK(character->Object.CurrentAction != PLAYER_STOP_FLY);
    }
}

TEST_CASE("Service NPC appearances: native Leo equipment models textures and wing animation load [service-npc]")
{
    if (std::getenv("MU_TEST_PRIVATE_SERVICE_GEAR") == nullptr)
    {
        MESSAGE("MU_TEST_SKIPPED: licensed native gear test inputs are not enabled");
        return;
    }
    MonsterRecorder::LoadScope scope(0);
    const auto changes = scope.Run(
        []
        {
            REQUIRE(gLoadData.AccessModel(MODEL_PLAYER, L"Data\\Player\\", L"player"));
            const int slots[] = {MODEL_BLACK_DRAGON_HELM, MODEL_BLACK_DRAGON_ARMOR, MODEL_BLACK_DRAGON_PANTS,
                                 MODEL_BLACK_DRAGON_GLOVES, MODEL_BLACK_DRAGON_BOOTS};
            const wchar_t* names[] = {L"HelmMale17", L"ArmorMale17", L"PantMale17", L"GloveMale17", L"BootMale17"};
            for (size_t i = 0; i < std::size(slots); ++i)
            {
                REQUIRE(gLoadData.AccessModel(slots[i], L"Data\\Player\\", names[i]));
                gLoadData.OpenTexture(slots[i], L"Player\\");
            }
            REQUIRE(gLoadData.AccessModel(MODEL_SWORD_OF_DESTRUCTION, L"Data\\Item\\", L"Sword17"));
            gLoadData.OpenTexture(MODEL_SWORD_OF_DESTRUCTION, L"Item\\");
            REQUIRE(gLoadData.AccessModel(MODEL_WING_OF_STORM, L"Data\\Item\\", L"wing08"));
            gLoadData.OpenTexture(MODEL_WING_OF_STORM, L"Item\\");
        });
    CHECK(changes.textureErrors.empty());
    const auto player = scope.ReadSlot(MODEL_PLAYER);
    CHECK(player.numBones == 60);
    CHECK(player.numActions > PLAYER_STOP_TWO_HAND_SWORD);
    for (int slot :
         {MODEL_BLACK_DRAGON_HELM, MODEL_BLACK_DRAGON_ARMOR, MODEL_BLACK_DRAGON_PANTS, MODEL_BLACK_DRAGON_GLOVES,
          MODEL_BLACK_DRAGON_BOOTS, MODEL_SWORD_OF_DESTRUCTION, MODEL_WING_OF_STORM})
    {
        const auto loaded = scope.ReadSlot(slot);
        CHECK(loaded.numMeshs > 0);
        CHECK(loaded.numBones > 0);
        CHECK(loaded.numActions > 0);
        for (const auto& texture : loaded.textures)
        {
            CHECK(texture.bitmap != L"unknown");
            // hide.jpg deliberately suppresses the original armour's duplicate skin mesh.
            CHECK((texture.bitmap != L"hidden" || texture.name == "hide.jpg"));
        }
    }
    const auto wing = scope.ReadSlot(MODEL_WING_OF_STORM);
    CHECK(wing.actions.front().keys > 1);
}

TEST_CASE("Service NPC appearances: Erin has an independent animated female model [service-npc]")
{
    if (!HasPrivateAssets())
        return;
    const int slot = MODEL_MONSTER01 + static_cast<int>(MONSTER_MODEL_SERVICE_ERIN);
    CHECK(MONSTER_MODEL_SERVICE_ERIN == 219);
    CHECK(Data::Monsters::FindMonsterModelSlot("MONSTER_MODEL_SERVICE_ERIN") == slot);
    MonsterRecorder::LoadScope load(0);
    const auto changes = load.Run([] { OpenMonsterModel(MONSTER_MODEL_SERVICE_ERIN); });
    CHECK(changes.textureErrors.empty());
    const auto model = load.ReadSlot(slot);
    CHECK(model.numMeshs == 2);
    CHECK(model.numBones == 49);
    CHECK(model.numActions == 2);
    CHECK(model.boneHead == 6);
    for (const auto& action : model.actions)
    {
        CHECK(action.keys == 21);
        CHECK(action.speed > 0.f);
    }
    for (const auto& texture : model.textures)
    {
        CHECK(texture.bitmap != L"unknown");
        CHECK(texture.bitmap != L"hidden");
    }
    MonsterRecorder::CreationScope creation(0);
    for (int i = 0; i < 2; ++i)
    {
        const CHARACTER* character = creation.Create(MONSTER_HELPER_ELLEN);
        REQUIRE(character != nullptr);
        CHECK(character->Object.Type == slot);
        CHECK(character->Object.Kind == KIND_NPC);
        CHECK(character->MonsterIndex == MONSTER_HELPER_ELLEN);
        CHECK(character->Object.CurrentAction < model.numActions);
        CHECK(character->Wing.Type == -1);
        CHECK_FALSE(Engine::Object::ServiceNpcs::HasTownHover(character));
    }
}
