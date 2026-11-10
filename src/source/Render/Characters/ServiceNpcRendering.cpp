#include "stdafx.h"

#include "ServiceNpcRendering.h"

#include "Data/GameData/MonsterData/ServiceNpcPresentation.h"
#include "Render/Models/ZzzBMD.h"

#include <algorithm>
#include <array>

namespace
{
namespace Presentation = Data::Monsters::ServiceNpcPresentation;
using Color = std::array<float, 3>;
constexpr int FaceMesh = 0;
constexpr int GemMesh = 10;
constexpr int ShoulderAndArmMesh = 1;
constexpr int ForearmMesh = 5;
constexpr int BodyMeshCount = 11;
// Native armor only: exclude the face, translucent decorations and cloth tabard.
constexpr int ArmorMeshes[] = {1, 4, 5, 7};
constexpr Color FaceLight{1.05f, 1.f, 0.97f};
constexpr Color BodyFill{0.85f, 0.85f, 0.9f};
constexpr Color ArmFill{1.f, 0.98f, 1.04f};
constexpr Color ArmShine{0.28f, 0.24f, 0.34f};
constexpr Color WingFill{0.85f, 0.85f, 0.9f};
constexpr Color ArmorShine{0.18f, 0.15f, 0.24f};
// Native reflective passes, with enough contrast to read as upgraded equipment.
constexpr Color WingChrome{0.28f, 0.24f, 0.34f};
constexpr Color WingMetal{0.22f, 0.20f, 0.28f};
constexpr Color WingChrome4{0.36f, 0.30f, 0.44f};
constexpr Color ExcellentPlatinum{0.28f, 0.25f, 0.34f};
constexpr Color ExcellentViolet{0.20f, 0.12f, 0.42f};
constexpr float ExcellentPulseRate = 0.002f;
constexpr Color GemLight{0.3f, 0.22f, 0.42f};
constexpr float ShinePulseRate = 0.001f;
constexpr float ShineBase = 0.85f;
constexpr float ShineRange = 0.15f;
constexpr int EnhancedQuality = 4;
constexpr int ExcellentMask = 0x3F;

class LightScope
{
public:
    explicit LightScope(BMD* model) : model_(model), enabled_(model->LightEnable)
    {
        VectorCopy(model->BodyLight, light_);
    }
    ~LightScope()
    {
        VectorCopy(light_, model_->BodyLight);
        model_->LightEnable = enabled_;
    }

private:
    BMD* model_;
    bool enabled_;
    vec3_t light_;
};

void SetLight(BMD* model, const Color& color, float strength = 1.f)
{
    for (int axis = 0; axis < 3; ++axis)
        model->BodyLight[axis] = color[axis] * strength;
}

float ShinePulse()
{
    return ShineBase + ShineRange * std::sin(WorldTime * ShinePulseRate);
}

void DrawMesh(BMD* model, int mesh, const DrawLook& look, int flags)
{
    if (mesh == look.hiddenMesh)
        return;
    model->RenderMesh(mesh, flags, look.alpha, look.blendMesh, look.blendMeshLight, look.blendMeshU, look.blendMeshV);
}

bool IsShoulderOrArm(int mesh)
{
    return mesh == ShoulderAndArmMesh || mesh == ForearmMesh;
}

void DrawBodyBase(BMD* model, const DrawLook& look, bool lit)
{
    Color fill;
    for (int axis = 0; axis < 3; ++axis)
        fill[axis] = std::clamp(model->BodyLight[axis], BodyFill[axis], 1.f);
    for (int mesh = 0; mesh < model->NumMeshs; ++mesh)
    {
        if (mesh == Presentation::AriaHaloMesh)
            continue;
        // A softly filled face must not inherit the terrain's dark directional lighting.
        model->LightEnable = mesh == FaceMesh || IsShoulderOrArm(mesh) ? false : lit;
        const Color& color = mesh == FaceMesh ? FaceLight : (IsShoulderOrArm(mesh) ? ArmFill : fill);
        SetLight(model, color);
        DrawMesh(model, mesh, look, RENDER_TEXTURE);
    }
}

void DrawArmorShine(BMD* model, const DrawLook& look)
{
    model->LightEnable = false;
    for (const int mesh : ArmorMeshes)
    {
        SetLight(model, IsShoulderOrArm(mesh) ? ArmShine : ArmorShine, ShinePulse());
        DrawMesh(model, mesh, look, RENDER_CHROME4 | RENDER_BRIGHT);
    }
    SetLight(model, GemLight, ShinePulse());
    DrawMesh(model, GemMesh, look, RENDER_TEXTURE | RENDER_BRIGHT);
}

void DrawWingLayer(BMD* model, const DrawLook& look, const Color& color, int flags)
{
    SetLight(model, color, ShinePulse());
    DrawMesh(model, 0, look, flags | RENDER_BRIGHT);
}

void DrawWingExcellent(BMD* model, const DrawLook& look)
{
    // Match the native excellent cycle's rate, retaining the private wing's platinum/violet palette.
    const float phase = 0.5f + 0.5f * std::sin(WorldTime * ExcellentPulseRate);
    Color color;
    for (int axis = 0; axis < 3; ++axis)
        color[axis] = ExcellentPlatinum[axis] + phase * (ExcellentViolet[axis] - ExcellentPlatinum[axis]);
    SetLight(model, color);
    DrawMesh(model, 0, look, RENDER_TEXTURE | RENDER_BRIGHT);
}

void DrawWingShine(BMD* model, const DrawLook& look, int level, int excellentFlags, int quality)
{
    if (quality == 0)
        return;
    const int effectiveLevel = quality < EnhancedQuality ? std::min(level, quality * 2 + 5) : level;
    if (effectiveLevel >= 7)
        DrawWingLayer(model, look, WingChrome, RENDER_CHROME);
    if (effectiveLevel >= 9)
        DrawWingLayer(model, look, WingMetal, RENDER_METAL);
    if (effectiveLevel >= 13)
        DrawWingLayer(model, look, WingChrome4, RENDER_CHROME4);
    if ((excellentFlags & ExcellentMask) != 0)
        DrawWingExcellent(model, look);
}
} // namespace

namespace Render::Characters::ServiceNpcRendering
{
bool DrawBody(BMD* model, const OBJECT* object, const DrawLook& look)
{
    if (object->Kind != KIND_NPC || object->Type != Presentation::AriaBodyModel || model->NumMeshs != BodyMeshCount)
        return false;
    if (look.hiddenMesh == -2 || look.alpha <= 0.f)
        return true;
    const LightScope restore(model);
    DrawBodyBase(model, look, model->LightEnable);
    DrawArmorShine(model, look);
    return true;
}

bool DrawHalo(BMD* model, const OBJECT* object, const DrawLook& look, vec34_t* bones, bool translate)
{
    if (object->Kind != KIND_NPC || object->Type != Presentation::AriaBodyModel || model->NumMeshs != BodyMeshCount ||
        model->NumBones <= Presentation::AriaHaloBone || bones == nullptr)
        return false;
    if (look.hiddenMesh == -2 || look.hiddenMesh == Presentation::AriaHaloMesh || look.alpha < 0.01f ||
        object->RenderType == RENDER_DARK)
        return true;
    // Wing and shadow passes change the shared skin state. Rebind the final body pose before this late mesh.
    model->BodyHeight = 0.f;
    model->BodyScale = object->Scale;
    VectorCopy(object->Position, model->BodyOrigin);
    model->Transform(bones, object->BoundingBoxMin, object->BoundingBoxMax, nullptr, translate);
    const LightScope restore(model);
    model->LightEnable = false;
    SetLight(model, BodyFill);
    DrawMesh(model, Presentation::AriaHaloMesh, look, RENDER_TEXTURE);
    return true;
}

bool DrawWing(BMD* model, int modelType, const DrawLook& look, int level, int excellentFlags, int quality,
              int renderType)
{
    if (modelType != Presentation::AriaWingModel || model->NumMeshs != 1 || (renderType & ~RENDER_TEXTURE) != 0)
        return false;
    if (look.hiddenMesh == -2 || look.alpha <= 0.f)
        return true;
    const LightScope restore(model);
    model->LightEnable = false;
    SetLight(model, WingFill);
    DrawMesh(model, 0, look, RENDER_TEXTURE);
    DrawWingShine(model, look, level, excellentFlags, quality);
    return true;
}
} // namespace Render::Characters::ServiceNpcRendering
