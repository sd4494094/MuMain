#include "stdafx.h"

#include "doctest.h"

#include "Engine/Object/MagicSkeletonCloth.h"
#include "Engine/Physics/PhysicsManager.h"
#include "../data/TestModelSlots.h"

#include <array>
#include <filesystem>
#include <memory>

namespace
{
constexpr int RobeMesh = 2;
constexpr int RobeBone = 18;

class Skeleton
{
public:
    Skeleton()
    {
        OBJECT& object = character.Object;
        object.Type = MODEL_MAGIC_SKELETON;
        object.Scale = 1.25f;
        object.BoneTransform = bones.data();
        for (auto& bone : bones)
            for (int axis = 0; axis < 3; ++axis)
                bone[axis][axis] = 1.f;
        Vector(100.f, 200.f, 300.f, object.Position);
    }
    ~Skeleton()
    {
        auto* cloth = static_cast<CPhysicsClothMesh*>(character.Object.m_pCloth);
        if (cloth != nullptr)
        {
            cloth[0].Destroy();
            delete[] cloth;
            character.Object.m_pCloth = nullptr;
        }
    }

    CHARACTER character{};
    std::array<vec34_t, MAX_BONES> bones{};
};

void OpenSkeleton()
{
    const std::wstring folder = (std::filesystem::path(MU_TEST_DATA_DIR) / "Monster").wstring() + L"/";
    REQUIRE(Models[MODEL_MAGIC_SKELETON].Open2(folder.c_str(), L"Monster63.bmd"));
}

void CheckPinnedVertices(Skeleton& skeleton)
{
    const BMD& model = Models[MODEL_MAGIC_SKELETON];
    const Mesh_t& mesh = model.Meshs[RobeMesh];
    const OBJECT& object = skeleton.character.Object;
    REQUIRE(object.m_pCloth != nullptr);
    const auto* cloth = static_cast<const CPhysicsClothMesh*>(object.m_pCloth);
    int pins = 0;
    for (int i = 0; i < mesh.NumVertices; ++i)
    {
        const Vertex_t& vertex = mesh.Vertices[i];
        if (vertex.Node != RobeBone)
            continue;
        ++pins;
        vec3_t actual;
        cloth[0].GetPosition(i, &actual);
        for (int axis = 0; axis < 3; ++axis)
        {
            const float expected =
                (vertex.Position[axis] + skeleton.bones[RobeBone][axis][3]) * object.Scale + object.Position[axis];
            CHECK(actual[axis] == doctest::Approx(expected));
        }
    }
    REQUIRE(pins > 0);
}
} // namespace

TEST_CASE("A Magic Skeleton creates its robe before its first draw [bloodcastle][cloth]")
{
    TestModelSlots slots;
    OpenSkeleton();
    auto skeleton = std::make_unique<Skeleton>();
    REQUIRE(Models[MODEL_MAGIC_SKELETON].m_pCurrentBoneTransform == nullptr);
    Engine::Object::MagicSkeletonCloth::Update(skeleton->character);
    REQUIRE(skeleton->character.Object.m_byNumCloth == 1);
    CheckPinnedVertices(*skeleton);
}

TEST_CASE("Shared Magic Skeleton models pin each robe to its own moving character [bloodcastle][cloth]")
{
    TestModelSlots slots;
    OpenSkeleton();
    auto first = std::make_unique<Skeleton>();
    auto second = std::make_unique<Skeleton>();
    Vector(1000.f, 2000.f, 3000.f, second->character.Object.Position);
    second->character.Object.Scale = 0.75f;
    for (int frame = 0; frame < 3; ++frame)
    {
        first->character.Object.Position[0] += 10.f;
        first->bones[RobeBone][1][3] += 5.f;
        second->character.Object.Position[1] -= 20.f;
        second->bones[RobeBone][2][3] -= 7.f;
        Engine::Object::MagicSkeletonCloth::Update(first->character);
        CheckPinnedVertices(*first);
        Engine::Object::MagicSkeletonCloth::Update(second->character);
        CheckPinnedVertices(*second);
    }
}

TEST_CASE("A Magic Skeleton waits for its pose before creating cloth [bloodcastle][cloth]")
{
    TestModelSlots slots;
    OpenSkeleton();
    auto skeleton = std::make_unique<Skeleton>();
    skeleton->character.Object.BoneTransform = nullptr;
    Engine::Object::MagicSkeletonCloth::Update(skeleton->character);
    CHECK(skeleton->character.Object.m_pCloth == nullptr);
    skeleton->character.Object.BoneTransform = skeleton->bones.data();
    Engine::Object::MagicSkeletonCloth::Update(skeleton->character);
    CheckPinnedVertices(*skeleton);
}
