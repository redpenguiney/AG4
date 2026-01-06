#include "gameobject.hpp"
#include "rendergroup.hpp"
#include <glm/gtx/quaternion.hpp>

Gameobject* Gameobject::New(GameobjectCreateParams params)
{
    return MemoryPool<Gameobject, GameobjectCreateParams>::Get().New(params);
}

Gameobject::Gameobject(GameobjectCreateParams params) {
    live = true;

    position = { 0, 0, 0 };
    rotation = glm::identity<glm::quat>();
    scale = { 1, 1, 1 };
    rotSclDirty = true;

    RenderGroup::FindRendergroupForGameobject(*this, params);
}

Gameobject::~Gameobject() {
    render.group->RemoveGameobject(*this);
    live = false;
}

void Gameobject::operator delete(void* obj) {
    return MemoryPool<Gameobject>::Get().Destroy(reinterpret_cast<Gameobject*>(obj));
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
}

void Gameobject::SetRotation(const glm::quat& r) {
    rotation = r;
    rotSclDirty = true;
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

//Physobject::Physobject(): Gameobject() {
//
//}