#include "stdafx.h"

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

namespace
{
constexpr int NoahNumber = 710;
constexpr int LeoNumber = 371;
// The wire identity of the stat reset NPC, independent of similarly named helpers.
constexpr int ErinNumber = 464;
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
    CHECK(lights == 11);
    CHECK(stars == 7);
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
            CHECK(draw.bodyLight[0] > 0.9f);
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

TEST_CASE("Service NPC rendering: private wing keeps +15 passes with bounded platinum violet light [service-npc]")
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
        CHECK(draw.bodyLight[0] < 0.1f);
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
