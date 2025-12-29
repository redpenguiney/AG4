#include "gameobject.hpp"

Gameobject* Gameobject::New()
{
    return MemoryPool<Gameobject>::Get().New();
}

Gameobject::Gameobject() {
    live = true;
}

Gameobject::~Gameobject() {
    //live = false;
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

Physobject::Physobject(): Gameobject() {

}