#pragma once
#include <vector>
#include <optional>
#include "glm/vec3.hpp"

struct Collision {
	// in space of objectA and objectB. That is, unscaled, unrotated, and untranslated..
	std::vector<std::pair<glm::vec3, glm::vec3>> collisionPoints;

	// B to A
	glm::vec3 collisionNormal;
};
class Gameobject;

std::optional<Collision> NarrowphaseCollisionDetection(Gameobject* a, Gameobject* b);
void ClearCollisionsCache();