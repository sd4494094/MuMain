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
constexpr Color WingShine{0.075f, 0.065f, 0.095f};
constexpr Color ExcellentShine{0.035f, 0.03f, 0.045f};
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

void DrawWingShine(BMD* model, const DrawLook& look, int level, int excellentFlags, int quality)
{
    if (quality == 0)
        return;
    const int effectiveLevel = quality < EnhancedQuality ? std::min(level, quality * 2 + 5) : level;
    if (effectiveLevel >= 7)
    {
        SetLight(model, WingShine, ShinePulse());
        DrawMesh(model, 0, look, RENDER_CHROME | RENDER_BRIGHT);
    }
    if (effectiveLevel >= 9)
    {
        SetLight(model, WingShine, ShinePulse());
        DrawMesh(model, 0, look, RENDER_METAL | RENDER_BRIGHT);
    }
    if (effectiveLevel >= 13)
    {
        SetLight(model, WingShine, ShinePulse());
        DrawMesh(model, 0, look, RENDER_CHROME4 | RENDER_BRIGHT);
    }
    if ((excellentFlags & ExcellentMask) != 0)
    {
        SetLight(model, ExcellentShine, ShinePulse());
        DrawMesh(model, 0, look, RENDER_TEXTURE | RENDER_BRIGHT);
    }
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
