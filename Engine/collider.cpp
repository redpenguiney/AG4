#include "collider.hpp"
#include "gameobject.hpp"

Collider::Collider(std::shared_ptr<BasePhysicsGeometry> m, Gameobject* obj):
physicsMesh(m),
object(obj)
{
    GameobjectSAS().Insert(this);
    Assert(node);
}

Collider::~Collider() {
	GameobjectSAS().Remove(this);
}

void Collider::UpdateAABB() {
    glm::vec3 size = object->Scale() * 0.5f;
    glm::vec3 max(0, 0, 0);
    for (float x : { -size.x, size.x }) {
        for (float y : { -size.y, size.y }) {
            for (float z : { -size.z, size.z }) {
                glm::vec3 transformed = object->Rotation() * glm::vec3(x, y, z);
                max.x = std::max(max.x, transformed.x);
                max.y = std::max(max.y, transformed.y);
                max.z = std::max(max.z, transformed.z);
            }
        }
    }

    glm::vec3 min = -max;
    aabb.min = glm::dvec3(min) + object->Position();
    aabb.max = glm::dvec3(max) + object->Position();

    //GameobjectSAS().UpdatePosition(this);
}
