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

    RenderGroup::FindRendergroupForGameobject(*this, params);
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
}

void Gameobject::SetScale(const glm::vec3& s) {
    scale = s;
    rotSclDirty = true;
    normalMatDirty = true;
}

void Gameobject::SetRotation(const glm::quat& r) {
    rotation = r;
    rotSclDirty = true;
    normalMatDirty = true;
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

Physobject::Physobject(const PhysobjectCreateParams& params): Gameobject(params) {

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
