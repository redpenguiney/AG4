#include "collider.hpp"
#include "gameobject.hpp"

Collider::Collider(std::shared_ptr<BasePhysicsMesh> m, Gameobject* obj):
physicsMesh(m),
object(obj)
{
    GameobjectSAS().Insert(this);
}

Collider::~Collider() {
	GameobjectSAS().Remove(this);
}

void Collider::UpdateAABB() {
    glm::vec3 min(-0.5f, -0.5f, -0.5f);
    for (float x : { -0.5f, 0.5f }) {
        for (float y : { -0.5f, 0.5f }) {
            for (float z : { -0.5f, 0.5f }) {
                glm::vec3 transformed = object->Rotation() * glm::vec3(x, y, z);
                min.x = std::min(min.x, transformed.x);
                min.y = std::min(min.y, transformed.y);
                min.z = std::min(min.z, transformed.z);
            }
        }
    }

    min *= object->Scale();
    glm::vec3 max = -min;
    aabb.min = min;
    aabb.max = max;

    GameobjectSAS().UpdatePosition(this);
}
