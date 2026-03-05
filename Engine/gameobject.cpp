#include "gameobject.hpp"
#include "rendergroup.hpp"
#include <glm/gtx/quaternion.hpp>
#include "static_meshpool.hpp"

Gameobject* Gameobject::New(const GameobjectCreateParams& params)
{
    return MemoryPool<Gameobject, const GameobjectCreateParams&>::Get().New(params);
}

Gameobject::Gameobject(const GameobjectCreateParams& params) {
    live = true;
    normalMatDirty = true;

    position = { 0, 0, 0 };
    rotation = glm::identity<glm::quat>();
    scale = { 1, 1, 1 };
    rotSclDirty = true;

    if (params.mesh) {
        RenderGroup::FindRendergroupForGameobject(*this, params);
    }
    else {
        meshpool = nullptr;
    }

    if (params.physicsMesh) {
        collider = std::make_unique<Collider>(params.physicsMesh, this);
    }
}

Gameobject::~Gameobject() {
    renderGroup->RemoveGameobject(*this);
    live = false;
}

void Gameobject::operator delete(void* obj) {
    MemoryPool<Gameobject, const GameobjectCreateParams&>::Get().Destroy(reinterpret_cast<Gameobject*>(obj));
}

const glm::dvec3& Gameobject::Position() const {
    return position;
}

const glm::vec3& Gameobject::Scale() const {
    return scale;
}

const glm::quat& Gameobject::Rotation() const {
    return rotation;
}

void Gameobject::SetPosition(const glm::dvec3& p) {
    position = p;
    if (collider) {
        collider->UpdateAABB();
        GameobjectSAS().UpdatePosition(collider.get());
    }
}

void Gameobject::SetScale(const glm::vec3& s) {
    scale = s;
    rotSclDirty = true;
    normalMatDirty = true;

    if (collider) {
        collider->UpdateAABB();
        GameobjectSAS().UpdatePosition(collider.get());
    }
}

void Gameobject::SetRotation(const glm::quat& r) {
    rotation = r;
    rotSclDirty = true;
    normalMatDirty = true;

    if (collider) {
        collider->UpdateAABB();
        GameobjectSAS().UpdatePosition(collider.get());
    }
}

void Gameobject::SetInstanceAttribute(const VertexAttribute& attrib, VertexScalar* value) {
    meshpool->SetInstancedVertexAttribute(drawInstanceIndex, attrib, value);
}

void Gameobject::SetInstanceAttribute(const VertexAttribute& attrib, glm::vec4 value) {
    Assert(attrib.nComponents == 4 && attrib.type == VertexScalarType::f32);
    SetInstanceAttribute(attrib, reinterpret_cast<VertexScalar*>(&value));
}

void Gameobject::SetInstanceAttribute(const VertexAttribute& attrib, float value) {
    Assert(attrib.nComponents == 1 && attrib.type == VertexScalarType::f32);
    SetInstanceAttribute(attrib, reinterpret_cast<VertexScalar*>(&value));
}

const glm::mat3x3& Gameobject::GetRotSclMatrix() {
    if (rotSclDirty) {
        rotSclMatrix = glm::identity<glm::mat3x3>();
        rotSclMatrix[0][0] = scale.x;
        rotSclMatrix[1][1] = scale.y;
        rotSclMatrix[2][2] = scale.z;
        rotSclMatrix = glm::toMat3(rotation) * rotSclMatrix;
        rotSclDirty = false;
    }
    return rotSclMatrix;
}

bool Gameobject::Live() {
    return live;
}

const Collider* const Gameobject::GetCollider() const {
    return collider.get();
}

glm::vec3 Gameobject::ObjectNormalToWorld(glm::vec3 normal) {
    const auto& rotscl = GetRotSclMatrix();
    return glm::normalize(
        normal.x / Scale().x * rotscl[0] +
        normal.y / Scale().y * rotscl[1] +
        normal.z / Scale().z * rotscl[2]
    );
}

Physobject::Physobject(const PhysobjectCreateParams& params): Gameobject(params) {
    velocity = { 0, 0, 0 };
    rotVelocity = { 0, 0, 0 };
}

void Physobject::UpdateMass() {

}

Physobject* Physobject::New(const PhysobjectCreateParams& params)
{
    return MemoryPool<Physobject, const PhysobjectCreateParams&>::Get().New(params);
}

void Physobject::operator delete(void* obj) {
    MemoryPool<Physobject, const PhysobjectCreateParams&>::Get().Destroy(reinterpret_cast<Physobject*>(obj));
}

Physobject::~Physobject() {

}

void Physobject::SetScale(const glm::vec3&) {
    UpdateMass();
}
