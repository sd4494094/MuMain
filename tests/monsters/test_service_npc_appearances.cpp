#include "stdafx.h"
#include "Camera/CameraState.h"
#include "Camera/CameraConfig.h"
#include "Engine/Object/CharacterAttachments.h"
#include "json.hpp"
#include "Camera/CameraProjection.h"
#include "Render/Characters/ServiceNpcHalo.h"

#include "MonsterCreationScope.h"
#include "MonsterModelRecorder.h"

#include "Core/Globals/_enum.h"
#include "Data/GameData/MonsterData/MonsterModelSlots.h"
#include "Data/DataHandler/MonsterData/MonsterJsonStorage.h"
#include "Engine/Object/ZzzOpenData.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ServiceNpcAppearance.h"
#include "Data/GameData/MonsterData/ServiceNpcPresentation.h"
#include "Data/GameData/ItemData/ItemModelJsonFormat.h"
#include "Render/Characters/ServiceNpcEffects.h"
#include "Render/Characters/ServiceNpcRendering.h"
#include "Render/Models/ModelDraw.h"
#include "Render/Effects/ZzzEffect.h"
#include "Render/Items/ItemGlow.h"
#include "../data/ItemModelDatabaseScope.h"
#include "Data/DataHandler/LoadData.h"
#include "Render/Models/ZzzBMD.h"

#include <doctest.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <limits>

namespace
{
constexpr int NoahNumber = 710;
constexpr int LeoNumber = 371;
// The wire identity of the stat reset NPC, independent of similarly named helpers.
constexpr int ErinNumber = 464;
CameraState HaloCamera(float yaw = 0.f)
{
    CameraState camera;
    Vector(0.f, -1100.f, 1000.f, camera.Position);
    Vector(-45.f, 0.f, yaw, camera.Angle);
    CameraProjection::UpdateView(camera, 0, 0, 1920, 1080);
    return camera;
}

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
    for (const int number : {414, static_cast<int>(MONSTER_LUKE_THE_HELPER)})
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
    CHECK(MONSTER_REINIT_HELPER == ErinNumber);
    CHECK(MONSTER_HELPER_ELLEN == 414);
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
        const CHARACTER* character = creation.Create(ErinNumber);
        REQUIRE(character != nullptr);
        CHECK(character->Object.Type == slot);
        CHECK(character->Object.Kind == KIND_NPC);
        CHECK(character->MonsterIndex == ErinNumber);
        CHECK(character->Object.CurrentAction < model.numActions);
        CHECK(character->Wing.Type == -1);
        CHECK_FALSE(Engine::Object::ServiceNpcs::HasTownHover(character));
    }
}

TEST_CASE("Service NPC bindings: shipped numeric identities create the model declared in client data [service-npc]")
{
    if (!HasPrivateAssets())
        return;
    const auto data =
        Data::Monsters::LoadMonstersFile(MonsterRecorder::ClientFolder() / "Data" / "Monsters" / "Monsters.json");
    REQUIRE(data.monsters.size() > 500);
    MonsterRecorder::CreationScope creation(0);
    for (const int number : {414, 464, 710, 711})
    {
        const auto entry = std::find_if(data.monsters.begin(), data.monsters.end(),
                                        [number](const auto& monster) { return monster.number == number; });
        REQUIRE(entry != data.monsters.end());
        const auto expected = Data::Monsters::FindMonsterModelReference(entry->model);
        REQUIRE(expected);
        const CHARACTER* character = creation.Create(number);
        REQUIRE(character != nullptr);
        CHECK(character->MonsterIndex == number);
        CHECK(character->Object.Type == *expected);
        CHECK(character->Object.Kind == KIND_NPC);
    }
}

TEST_CASE("Service NPC appearances: Aria uses independent female geometry and a separate excellent Illusion wing "
          "[service-npc]")
{
    CHECK(MONSTER_CREATOR_GODDESS_ARIA == 711);
    CHECK(MONSTER_MODEL_SERVICE_ARIA == 220);
    CHECK(MONSTER_MODEL_SERVICE_ERIN == 219);
    const int slot = MODEL_MONSTER01 + static_cast<int>(MONSTER_MODEL_SERVICE_ARIA);
    CHECK(Data::Monsters::FindMonsterModelSlot("MONSTER_MODEL_SERVICE_ARIA") == slot);
    if (!HasPrivateAssets())
        return;
    MonsterRecorder::LoadScope load(0);
    const auto changes = load.Run([] { OpenMonsterModel(MONSTER_MODEL_SERVICE_ARIA); });
    REQUIRE(changes.textureErrors.empty());
    const auto model = load.ReadSlot(slot);
    CHECK(model.numMeshs == 11);
    CHECK(model.numBones == 61);
    CHECK(model.numActions == 2);
    CHECK(model.boneHead == 20);
    for (const auto& action : model.actions)
    {
        CHECK(action.keys == 42);
        CHECK(action.speed > 0.f);
    }
    for (const auto& texture : model.textures)
    {
        CHECK(texture.bitmap != L"unknown");
        CHECK(texture.bitmap != L"hidden");
    }
    MonsterRecorder::CreationScope creation(0);
    for (int repeat = 0; repeat < 2; ++repeat)
    {
        CHARACTER* character = creation.Create(MONSTER_CREATOR_GODDESS_ARIA);
        REQUIRE(character != nullptr);
        CHECK(character->MonsterIndex == MONSTER_CREATOR_GODDESS_ARIA);
        CHECK(character->Object.Kind == KIND_NPC);
        CHECK(character->Object.Type == slot);
        CHECK(character->Object.BoundingBoxMax[2] == 280.f);
        CHECK(character->Object.CurrentAction < model.numActions);
        CHECK(character->Wing.Type == Data::Monsters::ServiceNpcPresentation::AriaWingModel);
        CHECK(character->Wing.Level == 15);
        CHECK(character->Wing.ExcellentFlags == 0x3F);
        CHECK(character->Object.BlendMesh == Data::Monsters::ServiceNpcPresentation::AriaHaloMesh);
        CHECK(Engine::Object::ServiceNpcs::PrepareDisplayWing(character));
        CHECK(character->Wing.LinkBone == 47);
        CHECK(character->Wing.PlaySpeed == doctest::Approx(0.3f));
        CHECK(character->Weapon[0].Type == -1);
        CHECK(character->BodyPart[BODYPART_ARMOR].Type == -1);
        CHECK_FALSE(Engine::Object::ServiceNpcs::HasTownHover(character));
    }
}

TEST_CASE(
    "Service NPC appearances: private wing preserves its native skeleton and resolves its own texture [service-npc]")
{
    if (!HasPrivateAssets())
        return;
    MonsterRecorder::LoadScope load(0);
    const auto changes = load.Run([] { OpenMonsterModel(MONSTER_MODEL_SERVICE_ARIA_WINGS); });
    REQUIRE(changes.textureErrors.empty());
    const auto wing = load.ReadSlot(Data::Monsters::ServiceNpcPresentation::AriaWingModel);
    CHECK(wing.numMeshs == 1);
    CHECK(wing.numBones == 39);
    CHECK(wing.boneHead == -1);
    REQUIRE(wing.actions.size() == 1);
    CHECK(wing.actions[0].keys == 8);
    CHECK(wing.actions[0].speed > 0.f);
    REQUIRE(wing.textures.size() == 1);
    CHECK(wing.textures[0].name == "aria_illusion.jpg");
    CHECK(wing.textures[0].bitmap != L"unknown");
}

TEST_CASE("Service NPC appearances: private +15 excellent glow does not change player wing glow rules [service-npc]")
{
    ItemModelDatabaseScope restoreModels;
    std::ifstream file(MonsterRecorder::ClientFolder() / "Data/Items/Models/Group12_Wing.json");
    REQUIRE(file.good());
    std::ostringstream text;
    text << file.rdbuf();
    std::vector<Data::Items::ItemModelDefinition> models;
    std::vector<Data::Items::ItemDataIssue> issues;
    Data::Items::ReadItemModelGroupJson(text.str(), "Group12_Wing.json", models, issues);
    REQUIRE(issues.empty());
    g_ItemModelDatabase.Build(models, Data::Effects::GlowColorList{});
    CHECK(Render::Items::Glow::GetLevel(MODEL_WING_OF_ILLUSION, 15) == 0);
    CHECK_FALSE(Render::Items::Glow::HasExcellentGlow(MODEL_WING_OF_ILLUSION));
    CHECK(Render::Items::Glow::GetLevel(Data::Monsters::ServiceNpcPresentation::AriaWingModel, 15) == 15);
    CHECK(Render::Items::Glow::HasExcellentGlow(Data::Monsters::ServiceNpcPresentation::AriaWingModel));
}

TEST_CASE(
    "Service NPC appearances: halo emits update sprites only for Aria and tolerates old asset bones [service-npc]")
{
    namespace Presentation = Data::Monsters::ServiceNpcPresentation;
    auto character = std::make_unique<CHARACTER>();
    auto bones = std::make_unique<vec34_t[]>(MAX_BONES);
    BMD model;
    model.NumBones = 61;
    model.NumMeshs = 11;
    model.BodyScale = 1.f;
    Vector(0.f, 0.f, 0.f, model.BodyOrigin);
    for (int axis = 0; axis < 3; ++axis)
        bones[Presentation::AriaHaloBone][axis][axis] = 1.f;
    character->MonsterIndex = MONSTER_CREATOR_GODDESS_ARIA;
    OBJECT& object = character->Object;
    object.Type = Presentation::AriaBodyModel;
    object.Kind = KIND_NPC;
    object.BoneTransform = bones.get();
    ClearSprites();
    REQUIRE(Render::Characters::ServiceNpcEffects::Update(character.get(), &object, &model));
    REQUIRE(Render::Characters::ServiceNpcEffects::UpdateHaloForView(character.get(), &object, &model, HaloCamera()));
    CHECK(object.BlendMesh == Presentation::AriaHaloMesh);
    CHECK(object.BlendMeshLight >= 0.75f);
    CHECK(object.BlendMeshLight <= 0.95f);
    int lights = 0;
    int stars = 0;
    for (int index = 0; index < MAX_SPRITES; ++index)
    {
        const auto& sprite = Sprites[index];
        if (!sprite.Live)
            continue;
        lights += sprite.Type == BITMAP_LIGHT ? 1 : 0;
        stars += sprite.Type == BITMAP_SHINY + 1 ? 1 : 0;
        for (const float component : sprite.Position)
            CHECK(std::isfinite(component));
    }
    CHECK(lights == 13);
    CHECK(stars == 9);
    ClearSprites();
    model.NumBones = 60;
    CHECK(Render::Characters::ServiceNpcEffects::Update(character.get(), &object, &model));
    character->MonsterIndex = MONSTER_LEVELING_WARP_GUIDE;
    CHECK_FALSE(Render::Characters::ServiceNpcEffects::Update(character.get(), &object, &model));
    CHECK_FALSE(Engine::Object::ServiceNpcs::PrepareDisplayWing(character.get()));
    for (int index = 0; index < MAX_SPRITES; ++index)
        CHECK_FALSE(Sprites[index].Live);
    object.BoneTransform = nullptr;
    // A BMD with metadata only owns no allocation in this unit test.
    model.NumBones = 0;
    model.NumMeshs = 0;
}

namespace
{
class AriaDrawRecorder final : public Render::ModelDrawing::ModelDrawer
{
public:
    AriaDrawRecorder()
    {
        Render::ModelDrawing::SetModelDrawer(this);
    }
    ~AriaDrawRecorder() override
    {
        Render::ModelDrawing::SetModelDrawer(nullptr);
    }
    void DrawMesh(const Render::ModelDrawing::MeshDraw& draw) override
    {
        draws.push_back(draw);
    }
    void DrawShadow(const Render::ModelDrawing::ShadowDraw&) override {}
    std::vector<Render::ModelDrawing::MeshDraw> draws;
};
} // namespace

TEST_CASE("Service NPC rendering: fill light and armor shine exclude the face cloth and hidden meshes [service-npc]")
{
    namespace Rendering = Render::Characters::ServiceNpcRendering;
    if (!HasPrivateAssets())
        return;
    MonsterRecorder::LoadScope load(0);
    load.Run([] { OpenMonsterModel(MONSTER_MODEL_SERVICE_ARIA); });
    BMD& model = Models[Data::Monsters::ServiceNpcPresentation::AriaBodyModel];
    model.LightEnable = true;
    Vector(0.2f, 0.3f, 0.4f, model.BodyLight);
    OBJECT object;
    object.Kind = KIND_NPC;
    object.Type = Data::Monsters::ServiceNpcPresentation::AriaBodyModel;
    DrawLook look;
    look.alpha = 0.5f;
    look.hiddenMesh = 5;
    AriaDrawRecorder recorder;
    ClearSprites();
    REQUIRE(Rendering::DrawBody(&model, &object, look));
    CHECK(recorder.draws.size() == 14);
    for (const auto& draw : recorder.draws)
    {
        CHECK(draw.mesh != 5);
        CHECK(draw.alpha == doctest::Approx(0.5f));
        if (draw.flags == RENDER_TEXTURE && draw.mesh == 0)
        {
            CHECK_FALSE(draw.lightEnable);
            CHECK(draw.bodyLight[0] == doctest::Approx(1.05f));
            CHECK(draw.bodyLight[1] == doctest::Approx(1.f));
            CHECK(draw.bodyLight[2] == doctest::Approx(0.97f));
        }
        if (draw.flags == RENDER_TEXTURE && draw.mesh == 1)
        {
            CHECK_FALSE(draw.lightEnable);
            CHECK(draw.bodyLight[0] == doctest::Approx(1.f));
            CHECK(draw.bodyLight[2] > 1.f);
        }
        if (draw.flags != RENDER_TEXTURE)
        {
            CHECK(draw.mesh != 0);
            CHECK(draw.mesh != 9);
            CHECK(draw.bodyLight[0] < 0.5f);
        }
    }
    CHECK(model.LightEnable);
    CHECK(model.BodyLight[0] == doctest::Approx(0.2f));
    CHECK(model.BodyLight[1] == doctest::Approx(0.3f));
    CHECK(model.BodyLight[2] == doctest::Approx(0.4f));
    recorder.draws.clear();
    look.hiddenMesh = -2;
    CHECK(Rendering::DrawBody(&model, &object, look));
    CHECK(recorder.draws.empty());
    look.hiddenMesh = -1;
    object.Kind = KIND_PLAYER;
    CHECK_FALSE(Rendering::DrawBody(&model, &object, look));
    object.Kind = KIND_NPC;
    object.Type = MODEL_MONSTER01;
    CHECK_FALSE(Rendering::DrawBody(&model, &object, look));
    for (int i = 0; i < MAX_SPRITES; ++i)
        CHECK_FALSE(Sprites[i].Live);
}

TEST_CASE("Service NPC rendering: private wing has visible +15 passes with bounded platinum violet light [service-npc]")
{
    namespace Rendering = Render::Characters::ServiceNpcRendering;
    const int wing = Data::Monsters::ServiceNpcPresentation::AriaWingModel;
    if (!HasPrivateAssets())
        return;
    MonsterRecorder::LoadScope load(0);
    load.Run([] { OpenMonsterModel(MONSTER_MODEL_SERVICE_ARIA_WINGS); });
    BMD& model = Models[wing];
    Vector(0.2f, 0.3f, 0.4f, model.BodyLight);
    DrawLook look;
    AriaDrawRecorder recorder;
    REQUIRE(Rendering::DrawWing(&model, wing, look, 15, 63, 4, RENDER_TEXTURE));
    REQUIRE(recorder.draws.size() == 5);
    CHECK(recorder.draws.front().flags == RENDER_TEXTURE);
    bool chrome = false, metal = false, chrome4 = false, excellent = false;
    for (size_t index = 1; index < recorder.draws.size(); ++index)
    {
        const auto& draw = recorder.draws[index];
        chrome |= draw.flags == (RENDER_CHROME | RENDER_BRIGHT);
        metal |= draw.flags == (RENDER_METAL | RENDER_BRIGHT);
        chrome4 |= draw.flags == (RENDER_CHROME4 | RENDER_BRIGHT);
        excellent |= draw.flags == (RENDER_TEXTURE | RENDER_BRIGHT);
        CHECK(draw.bodyLight[0] >= 0.12f);
        CHECK(draw.bodyLight[2] <= 0.45f);
        CHECK(draw.bodyLight[2] > draw.bodyLight[0]);
        CHECK(draw.bodyLight[0] > draw.bodyLight[1]);
    }
    CHECK(chrome);
    CHECK(metal);
    CHECK(chrome4);
    CHECK(excellent);
    CHECK(model.BodyLight[0] == doctest::Approx(0.2f));
    recorder.draws.clear();
    CHECK(Rendering::DrawWing(&model, wing, look, 15, 63, 0, RENDER_TEXTURE));
    CHECK(recorder.draws.size() == 1);
    recorder.draws.clear();
    CHECK(Rendering::DrawWing(&model, wing, look, 0, 0, 4, RENDER_TEXTURE));
    CHECK(recorder.draws.size() == 1);
    CHECK_FALSE(Rendering::DrawWing(&model, MODEL_WING_OF_ILLUSION, look, 15, 63, 4, RENDER_TEXTURE));
    CHECK_FALSE(Rendering::DrawWing(&model, wing, look, 15, 63, 4, RENDER_DARK));
    CHECK(recorder.draws.size() == 1);
}

TEST_CASE(
    "Service NPC effects: wing lights use animated anchors and never emit the native red green palette [service-npc]")
{
    namespace Presentation = Data::Monsters::ServiceNpcPresentation;
    auto bones = std::make_unique<vec34_t[]>(MAX_BONES);
    for (int bone = 0; bone < 39; ++bone)
        bones[bone][0][3] = static_cast<float>(bone);
    BMD model;
    model.NumBones = 39;
    model.BodyScale = 1.f;
    Vector(0.f, 0.f, 0.f, model.BodyOrigin);
    OBJECT object;
    ClearSprites();
    Render::Characters::ServiceNpcEffects::SpawnWing(&model, &object, Presentation::AriaWingModel, bones.get());
    int count = 0;
    for (int i = 0; i < MAX_SPRITES; ++i)
    {
        const auto& sprite = Sprites[i];
        if (!sprite.Live)
            continue;
        ++count;
        CHECK(sprite.Type != BITMAP_FLARE);
        CHECK(sprite.Light[2] > sprite.Light[0]);
        CHECK(sprite.Light[0] > sprite.Light[1]);
        CHECK(sprite.Position[0] >= 4.f);
        CHECK(sprite.Position[0] <= 38.f);
    }
    CHECK(count == 12);
    ClearSprites();
    model.NumBones = 38;
    Render::Characters::ServiceNpcEffects::SpawnWing(&model, &object, Presentation::AriaWingModel, bones.get());
    model.NumBones = 39;
    Render::Characters::ServiceNpcEffects::SpawnWing(&model, &object, MODEL_WING_OF_ILLUSION, bones.get());
    Render::Characters::ServiceNpcEffects::SpawnWing(&model, &object, Presentation::AriaWingModel, nullptr);
    for (int i = 0; i < MAX_SPRITES; ++i)
        CHECK_FALSE(Sprites[i].Live);
    model.NumBones = 0;
}

TEST_CASE("Service NPC animation: wing speed advances at thirty percent for different frame factors [service-npc]")
{
    if (!HasPrivateAssets())
        return;
    MonsterRecorder::CreationScope creation(0);
    CHARACTER* character = creation.Create(MONSTER_CREATOR_GODDESS_ARIA);
    REQUIRE(Engine::Object::ServiceNpcs::PrepareDisplayWing(character));
    BMD& wing = Models[character->Wing.Type];
    const float savedFactor = FPS_ANIMATION_FACTOR;
    for (const float factor : {0.5f, 1.f, 2.f})
    {
        FPS_ANIMATION_FACTOR = factor;
        float slow = 1.f, native = 1.f, slowPrior = 1.f, nativePrior = 1.f;
        unsigned short slowAction = 0, nativeAction = 0;
        vec3_t origin = {}, angle = {};
        wing.CurrentAction = 0;
        for (int tick = 0; tick < 2; ++tick)
        {
            wing.PlayAnimation(&slow, &slowPrior, &slowAction, character->Wing.PlaySpeed, origin, angle);
            wing.PlayAnimation(&native, &nativePrior, &nativeAction, 1.f, origin, angle);
        }
        CHECK((slow - 1.f) / (native - 1.f) == doctest::Approx(0.3f));
    }
    FPS_ANIMATION_FACTOR = savedFactor;
}

namespace
{
float MeshHeightBound(BMD& model, int mesh, const vec34_t* bones, bool maximum)
{
    float bound = maximum ? std::numeric_limits<float>::lowest() : std::numeric_limits<float>::max();
    const auto& geometry = model.Meshs[mesh];
    for (int index = 0; index < geometry.NumVertices; ++index)
    {
        const auto& vertex = geometry.Vertices[index];
        vec3_t position;
        VectorTransform(vertex.Position, bones[vertex.Node], position);
        bound = maximum ? std::max(bound, position[2]) : std::min(bound, position[2]);
    }
    return bound;
}
} // namespace

TEST_CASE(
    "Service NPC halo: actual overhead geometry and orbit stay clear of the head through both actions [service-npc]")
{
    if (!HasPrivateAssets())
        return;
    namespace Presentation = Data::Monsters::ServiceNpcPresentation;
    MonsterRecorder::LoadScope load(0);
    load.Run([] { OpenMonsterModel(MONSTER_MODEL_SERVICE_ARIA); });
    BMD& model = Models[Presentation::AriaBodyModel];
    auto bones = std::make_unique<vec34_t[]>(MAX_BONES);
    auto character = std::make_unique<CHARACTER>();
    character->MonsterIndex = MONSTER_CREATOR_GODDESS_ARIA;
    OBJECT& object = character->Object;
    object.Kind = KIND_NPC;
    object.Type = Presentation::AriaBodyModel;
    object.BoneTransform = bones.get();
    model.BodyScale = 1.f;
    Vector(0.f, 0.f, 0.f, model.BodyOrigin);
    vec3_t angle = {}, headAngle = {};
    for (int action = 0; action < model.NumActions; ++action)
    {
        model.CurrentAction = static_cast<unsigned short>(action);
        // Fractional samples also cover interpolated head movement, rather than keyframes only.
        for (float frame = 0.f; frame < model.Actions[action].NumAnimationKeys; frame += 0.5f)
        {
            model.Animation(bones.get(), frame, frame, static_cast<unsigned short>(action), angle, headAngle, false,
                            true, nullptr);
            const float headTop = MeshHeightBound(model, 0, bones.get(), true);
            const float haloBottom = MeshHeightBound(model, Presentation::AriaHaloMesh, bones.get(), false);
            CHECK(haloBottom - headTop >= 12.f);
            ClearSprites();
            REQUIRE(Render::Characters::ServiceNpcEffects::Update(character.get(), &object, &model));
            REQUIRE(Render::Characters::ServiceNpcEffects::UpdateHaloForView(character.get(), &object, &model,
                                                                             HaloCamera()));
            int overheadSprites = 0;
            for (int index = 0; index < MAX_SPRITES; ++index)
            {
                const auto& sprite = Sprites[index];
                if (!sprite.Live)
                    continue;
                const float x = sprite.Position[0] - bones[20][0][3];
                const float y = sprite.Position[1] - bones[20][1][3];
                const float z = sprite.Position[2] - bones[20][2][3];
                CHECK(x * x + y * y + z * z > 15.f * 15.f);
                if (sprite.Position[2] > headTop + 12.f)
                {
                    ++overheadSprites;
                    float radiusSquared = 0.f;
                    for (int axis = 0; axis < 3; ++axis)
                    {
                        const float delta = sprite.Position[axis] - bones[Presentation::AriaHaloBone][axis][3];
                        radiusSquared += delta * delta;
                    }
                    CHECK(std::sqrt(radiusSquared) == doctest::Approx(19.f));
                }
            }
            CHECK(overheadSprites == 12);
        }
    }
    object.BoneTransform = nullptr;
    ClearSprites();
}

TEST_CASE(
    "Service NPC armor: both arm meshes get local fill while the face and cloth keep their own material [service-npc]")
{
    if (!HasPrivateAssets())
        return;
    MonsterRecorder::LoadScope load(0);
    load.Run([] { OpenMonsterModel(MONSTER_MODEL_SERVICE_ARIA); });
    BMD& model = Models[Data::Monsters::ServiceNpcPresentation::AriaBodyModel];
    model.LightEnable = true;
    Vector(0.15f, 0.15f, 0.15f, model.BodyLight);
    OBJECT object;
    object.Kind = KIND_NPC;
    object.Type = Data::Monsters::ServiceNpcPresentation::AriaBodyModel;
    DrawLook look;
    AriaDrawRecorder recorder;
    REQUIRE(Render::Characters::ServiceNpcRendering::DrawBody(&model, &object, look));
    int armBases = 0, armSheens = 0;
    for (const auto& draw : recorder.draws)
    {
        CHECK((draw.flags & RENDER_NODEPTH) == 0);
        if (draw.mesh == 1 || draw.mesh == 5)
        {
            CHECK_FALSE(draw.lightEnable);
            if (draw.flags == RENDER_TEXTURE)
            {
                ++armBases;
                CHECK(draw.bodyLight[0] == doctest::Approx(1.f));
            }
            else
            {
                ++armSheens;
                CHECK(draw.bodyLight[0] >= 0.19f);
                CHECK(draw.bodyLight[2] <= 0.34f);
            }
        }
        if (draw.mesh == 9)
        {
            CHECK(draw.flags == RENDER_TEXTURE);
            CHECK(draw.lightEnable);
            CHECK(draw.bodyLight[0] == doctest::Approx(0.85f));
        }
    }
    CHECK(armBases == 2);
    CHECK(armSheens == 2);
    CHECK(model.LightEnable);
    CHECK(model.BodyLight[0] == doctest::Approx(0.15f));
}

namespace
{
void CheckHaloAfterHeadTurn(BMD& model, vec34_t* bones, const vec34_t* reference, float frame, unsigned short action,
                            const vec3_t bodyAngle, const vec3_t headAngle)
{
    namespace Presentation = Data::Monsters::ServiceNpcPresentation;
    model.Animation(bones, frame, frame, action, bodyAngle, headAngle, false, true, nullptr);
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 4; ++column)
            CHECK(std::abs(bones[Presentation::AriaHaloBone][row][column] -
                           reference[Presentation::AriaHaloBone][row][column]) < 0.002f);
    const float headTop = MeshHeightBound(model, 0, bones, true);
    const float haloBottom = MeshHeightBound(model, Presentation::AriaHaloMesh, bones, false);
    CHECK(haloBottom - headTop >= 12.f);
    // The player-visible center is the torso axis, not the independently offset head joint.
    constexpr int PelvisBone = 2;
    constexpr int ChestBone = 18;
    constexpr int HeadBone = 20;
    const float torsoX = (reference[PelvisBone][0][3] + reference[ChestBone][0][3]) * 0.5f;
    const float torsoY = (reference[PelvisBone][1][3] + reference[ChestBone][1][3]) * 0.5f;
    const float x = bones[Presentation::AriaHaloBone][0][3] - torsoX;
    const float y = bones[Presentation::AriaHaloBone][1][3] - torsoY;
    const float z = bones[Presentation::AriaHaloBone][2][3] - reference[HeadBone][2][3];
    CHECK(x * x + y * y < 0.01f);
    CHECK(z >= 51.5f);
    CHECK(z <= 52.5f);
}
} // namespace

TEST_CASE("Service NPC halo: torso axis stays aligned at every facing and half frame [service-npc]")
{
    if (!HasPrivateAssets())
        return;
    MonsterRecorder::LoadScope load(0);
    load.Run([] { OpenMonsterModel(MONSTER_MODEL_SERVICE_ARIA); });
    BMD& model = Models[Data::Monsters::ServiceNpcPresentation::AriaBodyModel];
    auto bones = std::make_unique<vec34_t[]>(MAX_BONES);
    auto reference = std::make_unique<vec34_t[]>(MAX_BONES);
    const auto& halo = model.Meshs[Data::Monsters::ServiceNpcPresentation::AriaHaloMesh];
    for (int index = 0; index < halo.NumVertices; ++index)
    {
        const auto& vertex = halo.Vertices[index];
        CHECK(vertex.Node == Data::Monsters::ServiceNpcPresentation::AriaHaloBone);
        CHECK(std::hypot(vertex.Position[0], vertex.Position[1]) <= 25.f);
    }
    model.BodyScale = 1.f;
    Vector(0.f, 0.f, 0.f, model.BodyOrigin);
    const float headTurns[][2] = {{0.f, 0.f},   {-64.f, -16.f}, {-64.f, 16.f}, {64.f, -16.f},
                                  {64.f, 16.f}, {0.f, -16.f},   {0.f, 16.f}};
    vec3_t straight = {};
    for (int action = 0; action < model.NumActions; ++action)
    {
        model.CurrentAction = static_cast<unsigned short>(action);
        for (const float facing : {0.f, 90.f, 180.f, 270.f})
        {
            vec3_t bodyAngle = {0.f, 0.f, facing};
            for (float frame = 0.f; frame < model.Actions[action].NumAnimationKeys; frame += 0.5f)
            {
                model.Animation(reference.get(), frame, frame, model.CurrentAction, bodyAngle, straight, false, true,
                                nullptr);
                for (const auto& turn : headTurns)
                {
                    vec3_t headAngle = {turn[0], turn[1], 0.f};
                    CheckHaloAfterHeadTurn(model, bones.get(), reference.get(), frame, model.CurrentAction, bodyAngle,
                                           headAngle);
                }
            }
        }
    }
}

TEST_CASE("Service NPC rendering: wing excellent cycles color and respects every quality setting [service-npc]")
{
    if (!HasPrivateAssets())
        return;
    namespace Rendering = Render::Characters::ServiceNpcRendering;
    const int wing = Data::Monsters::ServiceNpcPresentation::AriaWingModel;
    MonsterRecorder::LoadScope load(0);
    load.Run([] { OpenMonsterModel(MONSTER_MODEL_SERVICE_ARIA_WINGS); });
    BMD& model = Models[wing];
    model.LightEnable = true;
    Vector(0.2f, 0.3f, 0.4f, model.BodyLight);
    DrawLook look;
    look.alpha = 0.6f;
    AriaDrawRecorder recorder;
    const size_t qualityDraws[] = {1, 3, 4, 4, 5};
    for (int quality = 0; quality <= 4; ++quality)
    {
        recorder.draws.clear();
        REQUIRE(Rendering::DrawWing(&model, wing, look, 15, 63, quality, RENDER_TEXTURE));
        CHECK(recorder.draws.size() == qualityDraws[quality]);
        for (const auto& draw : recorder.draws)
        {
            CHECK(draw.alpha == doctest::Approx(look.alpha));
            CHECK((draw.flags & RENDER_NODEPTH) == 0);
        }
    }
    const float savedTime = WorldTime;
    float excellentRed[2] = {};
    int index = 0;
    for (const float time : {785.398163f, 2356.19449f})
    {
        recorder.draws.clear();
        WorldTime = time;
        const bool drawn = Rendering::DrawWing(&model, wing, look, 15, 63, 4, RENDER_TEXTURE);
        WorldTime = savedTime;
        REQUIRE(drawn);
        REQUIRE(recorder.draws.size() == 5);
        const auto& excellent = recorder.draws.back();
        excellentRed[index++] = excellent.bodyLight[0];
        CHECK(excellent.bodyLight[0] >= 0.19f);
        CHECK(excellent.bodyLight[2] <= 0.43f);
        CHECK(model.LightEnable);
        CHECK(model.BodyLight[0] == doctest::Approx(0.2f));
    }
    CHECK(std::abs(excellentRed[0] - excellentRed[1]) > 0.07f);
}

namespace
{
void HaloPoint(const BMD& model, const vec34_t* bones, int bone, bool world, vec3_t result)
{
    for (int axis = 0; axis < 3; ++axis)
        result[axis] = world ? bones[bone][axis][3] : bones[bone][axis][3] * model.BodyScale + model.BodyOrigin[axis];
}

float CheckHaloScreenSample(const OBJECT& object, BMD& model, vec34_t* bones, const CameraState& camera, bool world)
{
    namespace Presentation = Data::Monsters::ServiceNpcPresentation;
    auto original = std::make_unique<vec34_t[]>(MAX_BONES);
    std::memcpy(original.get(), bones, MAX_BONES * sizeof(vec34_t));
    vec3_t pelvis, chest, torso, halo;
    HaloPoint(model, bones, 2, world, pelvis);
    HaloPoint(model, bones, 18, world, chest);
    for (int axis = 0; axis < 3; ++axis)
        torso[axis] = (pelvis[axis] + chest[axis]) * 0.5f;
    HaloPoint(model, bones, Presentation::AriaHaloBone, world, halo);
    float torsoX, torsoY, haloX, haloY;
    REQUIRE(CameraProjection::WorldToWindowPixel(camera, torso, &torsoX, &torsoY));
    REQUIRE(CameraProjection::WorldToWindowPixel(camera, halo, &haloX, &haloY));
    const float beforeError = std::abs(torsoX - haloX);
    REQUIRE(Render::Characters::ServiceNpcHalo::Align(object, model, bones, camera, world));
    HaloPoint(model, bones, Presentation::AriaHaloBone, world, halo);
    REQUIRE(CameraProjection::WorldToWindowPixel(camera, halo, &haloX, &haloY));
    CHECK(std::abs(torsoX - haloX) < 0.03f);
    CHECK(haloY < torsoY);
    CHECK(std::isfinite(halo[2]));
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 3; ++column)
            CHECK(bones[Presentation::AriaHaloBone][row][column] == original[Presentation::AriaHaloBone][row][column]);
    for (int bone = 0; bone < Presentation::AriaHaloBone; ++bone)
        CHECK(std::memcmp(bones[bone], original[bone], sizeof(vec34_t)) == 0);
    const float alignedX = bones[Presentation::AriaHaloBone][0][3];
    const float alignedY = bones[Presentation::AriaHaloBone][1][3];
    REQUIRE(Render::Characters::ServiceNpcHalo::Align(object, model, bones, camera, world));
    CHECK(bones[Presentation::AriaHaloBone][0][3] == alignedX);
    CHECK(bones[Presentation::AriaHaloBone][1][3] == alignedY);
    return beforeError;
}
} // namespace

TEST_CASE("Service NPC halo: final perspective centers the halo anchor at screen edges [service-npc]")
{
    if (!HasPrivateAssets())
        return;
    namespace Presentation = Data::Monsters::ServiceNpcPresentation;
    MonsterRecorder::LoadScope load(0);
    load.Run([] { OpenMonsterModel(MONSTER_MODEL_SERVICE_ARIA); });
    BMD& model = Models[Presentation::AriaBodyModel];
    auto bones = std::make_unique<vec34_t[]>(MAX_BONES);
    OBJECT object;
    object.Kind = KIND_NPC;
    object.Type = Presentation::AriaBodyModel;
    float maximumBeforeError = 0.f;
    for (const bool world : {false, true})
        for (const float scale : {0.7f, 1.f, 1.5f})
            for (const float side : {-600.f, 0.f, 600.f})
                for (const float facing : {0.f, 90.f, 180.f, 270.f})
                    for (const float yaw : {-20.f, 0.f, 20.f})
                    {
                        const CameraState camera = HaloCamera(yaw);
                        model.BodyScale = scale;
                        Vector(side, 0.f, 0.f, model.BodyOrigin);
                        for (int action = 0; action < model.NumActions; ++action)
                        {
                            model.CurrentAction = static_cast<unsigned short>(action);
                            for (float frame = 0.f; frame < model.Actions[action].NumAnimationKeys; frame += 0.5f)
                            {
                                vec3_t angle = {0.f, 0.f, facing}, head = {64.f, 16.f, 0.f};
                                model.Animation(bones.get(), frame, frame, model.CurrentAction, angle, head, false,
                                                world);
                                maximumBeforeError =
                                    std::max(maximumBeforeError,
                                             CheckHaloScreenSample(object, model, bones.get(), camera, world));
                            }
                        }
                    }
    CHECK(maximumBeforeError > 20.f);
    MESSAGE("maximum uncorrected screen error: ", maximumBeforeError);
}

TEST_CASE("Service NPC halo: invalid views and unrelated models remain unchanged [service-npc]")
{
    namespace Presentation = Data::Monsters::ServiceNpcPresentation;
    OBJECT object;
    object.Kind = KIND_NPC;
    object.Type = Presentation::AriaBodyModel;
    BMD model;
    model.NumBones = Presentation::AriaHaloBone + 1;
    model.BodyScale = 1.f;
    Vector(0.f, 0.f, 0.f, model.BodyOrigin);
    auto bones = std::make_unique<vec34_t[]>(MAX_BONES);
    const CameraState unset;
    CHECK_FALSE(Render::Characters::ServiceNpcHalo::Align(object, model, bones.get(), unset, false));
    CameraState behind = HaloCamera();
    for (int axis = 0; axis < 3; ++axis)
        behind.Matrix[2][axis] = -behind.Matrix[2][axis];
    behind.Matrix[2][3] = -behind.Matrix[2][3];
    CHECK_FALSE(Render::Characters::ServiceNpcHalo::Align(object, model, bones.get(), behind, false));
    object.Type = MODEL_PLAYER;
    CHECK_FALSE(Render::Characters::ServiceNpcHalo::Align(object, model, bones.get(), HaloCamera(), false));
    model.NumBones = 0;
}

namespace
{
namespace HaloPresentation = Data::Monsters::ServiceNpcPresentation;
namespace HaloAttachments = Engine::Object::Attachments;

CameraState NativeHaloCamera(float yaw = -45.f)
{
    constexpr float Distance = 1735.f;
    constexpr float CameraHeightOffset = 150.f;
    const float angle = yaw * Q_PI / 180.f;
    CameraState camera;
    Vector(-Distance * std::sin(angle), -Distance * std::cos(angle), Distance - CameraHeightOffset, camera.Position);
    Vector(-48.5f, 0.f, yaw, camera.Angle);
    camera.FOV = HFovToVFov(40.f, 1920.f / 1080.f);
    CameraProjection::UpdateView(camera, 0, 0, 1920, 1080);
    return camera;
}

void VertexEye(const Vertex_t& vertex, const vec34_t* bones, float scale, const vec3_t origin,
               const CameraState& camera, vec3_t eye)
{
    vec3_t point;
    VectorTransform(vertex.Position, bones[vertex.Node], point);
    for (int axis = 0; axis < 3; ++axis)
        point[axis] = point[axis] * scale + origin[axis];
    VectorTransform(point, camera.Matrix, eye);
}

float MeshFront(const BMD& model, int meshIndex, const vec34_t* bones, float scale, const vec3_t origin,
                const CameraState& camera)
{
    float front = std::numeric_limits<float>::lowest();
    const auto& mesh = model.Meshs[meshIndex];
    for (int index = 0; index < mesh.NumVertices; ++index)
    {
        vec3_t eye;
        VertexEye(mesh.Vertices[index], bones, scale, origin, camera, eye);
        front = std::max(front, eye[2]);
    }
    return front;
}

float MeshVerticalRatio(const BMD& model, int meshIndex, const vec34_t* bones, float scale, const vec3_t origin,
                        const CameraState& camera, bool maximum)
{
    float bound = maximum ? std::numeric_limits<float>::lowest() : std::numeric_limits<float>::max();
    const auto& mesh = model.Meshs[meshIndex];
    for (int index = 0; index < mesh.NumVertices; ++index)
    {
        vec3_t eye;
        VertexEye(mesh.Vertices[index], bones, scale, origin, camera, eye);
        const float ratio = eye[1] / -eye[2];
        bound = maximum ? std::max(bound, ratio) : std::min(bound, ratio);
    }
    return bound;
}

void PoseHaloWing(BMD& body, const vec34_t* bodyBones, BMD& wingModel, HaloAttachments::Slot& wing, float frame)
{
    wing.type = HaloPresentation::AriaWingModel;
    wing.posed.scale = body.BodyScale * HaloPresentation::AriaWingScale;
    vec3_t offset = {0.f, 0.f, HaloPresentation::AriaWingOffsetZ}, rotated, angle = {};
    VectorRotate(offset, bodyBones[47], rotated);
    for (int axis = 0; axis < 3; ++axis)
        wing.posed.origin[axis] = body.BodyOrigin[axis] + rotated[axis] * body.BodyScale;
    wingModel.CurrentAction = 0;
    wingModel.Animation(wing.bones.get(), frame, frame, 0, angle, angle, true, true, bodyBones[47]);
}

nlohmann::json HaloMatrices(const vec34_t* bones, int count)
{
    auto result = nlohmann::json::array();
    for (int bone = 0; bone < count; ++bone)
        result.push_back({{bones[bone][0][0], bones[bone][0][1], bones[bone][0][2], bones[bone][0][3]},
                          {bones[bone][1][0], bones[bone][1][1], bones[bone][1][2], bones[bone][1][3]},
                          {bones[bone][2][0], bones[bone][2][1], bones[bone][2][2], bones[bone][2][3]}});
    return result;
}

void ExportHaloPose(const char* label, const BMD& body, const vec34_t* bones, const HaloAttachments::Slot& wing,
                    const CameraState& camera)
{
    const char* folder = std::getenv("MU_ARIA_VISIBILITY_EXPORT");
    if (folder == nullptr)
        return;
    nlohmann::json data = {{"body", HaloMatrices(bones, body.NumBones)},
                           {"wing", HaloMatrices(wing.bones.get(), HaloPresentation::AriaWingBoneCount)},
                           {"scale", body.BodyScale},
                           {"origin", {body.BodyOrigin[0], body.BodyOrigin[1], body.BodyOrigin[2]}},
                           {"wingScale", wing.posed.scale},
                           {"wingOrigin", {wing.posed.origin[0], wing.posed.origin[1], wing.posed.origin[2]}},
                           {"camera", HaloMatrices(&camera.Matrix, 1)[0]},
                           {"perspectiveX", camera.PerspectiveX},
                           {"perspectiveY", camera.PerspectiveY}};
    std::ofstream output(std::string(folder) + "/" + label + ".json");
    REQUIRE(output.good());
    output << data.dump(2);
}
} // namespace

TEST_CASE("Service NPC halo: actual head and animated wings cannot cover the ring [service-npc]")
{
    if (!HasPrivateAssets())
        return;
    MonsterRecorder::LoadScope load(0);
    load.Run(
        []
        {
            OpenMonsterModel(MONSTER_MODEL_SERVICE_ARIA);
            OpenMonsterModel(MONSTER_MODEL_SERVICE_ARIA_WINGS);
        });
    BMD& body = Models[HaloPresentation::AriaBodyModel];
    BMD& wingModel = Models[HaloPresentation::AriaWingModel];
    auto bones = std::make_unique<vec34_t[]>(MAX_BONES);
    HaloAttachments::Slot wing;
    wing.bones = std::make_unique<vec34_t[]>(MAX_BONES);
    OBJECT object;
    object.Kind = KIND_NPC;
    object.Type = HaloPresentation::AriaBodyModel;
    int samples = 0;
    int previousDepthFailures = 0;
    float minimumCrownGap = std::numeric_limits<float>::max();
    for (const float scale : {0.7f, 1.5f})
        for (const float side : {-450.f, 0.f, 450.f})
            for (const float facing : {0.f, 45.f, 90.f, 135.f, 180.f, 225.f, 270.f, 315.f})
                for (const float yaw : {-65.f, -45.f, -25.f})
                    for (int action = 0; action < body.NumActions; ++action)
                        for (const float bodyFrame : {0.f, 0.5f, 20.5f})
                            for (const float wingFrame : {0.f, 0.5f, 1.f, 2.5f})
                            {
                                CameraState camera = NativeHaloCamera(yaw);
                                body.BodyScale = scale;
                                Vector(side, 0.f, 0.f, body.BodyOrigin);
                                vec3_t angle = {0.f, 0.f, facing},
                                       head = {bodyFrame == 0.f ? -64.f : 64.f, bodyFrame == 0.5f ? -16.f : 16.f};
                                body.CurrentAction = static_cast<unsigned short>(action);
                                body.Animation(bones.get(), bodyFrame, bodyFrame, body.CurrentAction, angle, head,
                                               false, false);
                                PoseHaloWing(body, bones.get(), wingModel, wing, wingFrame);
                                const float wingFront = MeshFront(wingModel, 0, wing.bones.get(), wing.posed.scale,
                                                                  wing.posed.origin, camera);
                                const float previousRingFront = MeshFront(body, HaloPresentation::AriaHaloMesh,
                                                                          bones.get(), scale, body.BodyOrigin, camera);
                                previousDepthFailures += previousRingFront < wingFront ? 1 : 0;
                                REQUIRE(Render::Characters::ServiceNpcHalo::Align(object, body, bones.get(), camera,
                                                                                  false, &wing));
                                const float headTop =
                                    MeshVerticalRatio(body, 0, bones.get(), scale, body.BodyOrigin, camera, true);
                                const float haloBottom =
                                    MeshVerticalRatio(body, HaloPresentation::AriaHaloMesh, bones.get(), scale,
                                                      body.BodyOrigin, camera, false);
                                const float gap = (haloBottom - headTop) / camera.PerspectiveY;
                                minimumCrownGap = std::min(minimumCrownGap, gap);
                                CHECK(gap > 0.5f);
                                const auto& mesh = body.Meshs[HaloPresentation::AriaHaloMesh];
                                float nearest = std::numeric_limits<float>::max();
                                for (int index = 0; index < mesh.NumVertices; ++index)
                                {
                                    vec3_t eye;
                                    VertexEye(mesh.Vertices[index], bones.get(), scale, body.BodyOrigin, camera, eye);
                                    nearest = std::min(nearest, eye[2]);
                                }
                                CHECK(nearest > wingFront + 0.5f * scale);
                                ++samples;
                            }
    CHECK(samples == 3456);
    CHECK(previousDepthFailures > 0);
    MESSAGE("visibility samples: ", samples, "; minimum projected crown gap: ", minimumCrownGap,
            "; old depth separation failures: ", previousDepthFailures);
}

TEST_CASE("Service NPC halo: native camera preview retains ring geometry and the selected facing [service-npc]")
{
    if (!HasPrivateAssets())
        return;
    MonsterRecorder::LoadScope load(0);
    load.Run(
        []
        {
            OpenMonsterModel(MONSTER_MODEL_SERVICE_ARIA);
            OpenMonsterModel(MONSTER_MODEL_SERVICE_ARIA_WINGS);
        });
    BMD& body = Models[HaloPresentation::AriaBodyModel];
    BMD& wingModel = Models[HaloPresentation::AriaWingModel];
    auto bones = std::make_unique<vec34_t[]>(MAX_BONES);
    HaloAttachments::Slot wing;
    wing.bones = std::make_unique<vec34_t[]>(MAX_BONES);
    OBJECT object;
    object.Kind = KIND_NPC;
    object.Type = HaloPresentation::AriaBodyModel;
    const auto& mesh = body.Meshs[HaloPresentation::AriaHaloMesh];
    for (int index = 0; index < mesh.NumVertices; ++index)
    {
        const auto& v = mesh.Vertices[index];
        CHECK(v.Node == HaloPresentation::AriaHaloBone);
        CHECK(std::abs(v.Position[0]) <= 24.001f);
        CHECK(std::abs(v.Position[1]) <= 24.001f);
        CHECK(std::abs(v.Position[2]) <= 1.201f);
    }
    // Server direction 2/4 -> wire 1/3 -> client (wire - 1) * 45 = 0/90 degrees.
    for (const float facing : {0.f, 90.f})
    {
        CameraState camera = NativeHaloCamera();
        body.BodyScale = 1.f;
        Vector(0.f, 200.f, 0.f, body.BodyOrigin);
        vec3_t angle = {0.f, 0.f, facing}, head = {};
        body.CurrentAction = 0;
        body.Animation(bones.get(), 0.f, 0.f, 0, angle, head, false, false);
        PoseHaloWing(body, bones.get(), wingModel, wing, 0.f);
        ExportHaloPose(facing == 0.f ? "southwest-before" : "southeast-before", body, bones.get(), wing, camera);
        REQUIRE(Render::Characters::ServiceNpcHalo::Align(object, body, bones.get(), camera, false, &wing));
        ExportHaloPose(facing == 0.f ? "southwest-after" : "southeast-after", body, bones.get(), wing, camera);
    }
}
