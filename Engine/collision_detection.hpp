#pragma once
#include <vector>
#include <optional>
#include "glm/vec3.hpp"

struct Collision {
	std::vector<glm::dvec3> collisionPointsA;
	std::vector<glm::dvec3> collisionPointsB;
	glm::vec3 collisionNormal;
};
class Gameobject;

std::optional<Collision> NarrowphaseCollisionDetection(Gameobject* a, Gameobject* b);
void ClearCollisionsCache();