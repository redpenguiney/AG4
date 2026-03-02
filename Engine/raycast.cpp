#include "raycast.hpp"
#include "aabb_tree.hpp"
#include "collider.hpp"
#include "gameobject.hpp"

RaycastResult Raycast(glm::dvec3 origin, glm::dvec3 direction, RaycastParams params) {
	auto hitColliders = GameobjectSAS().QueryRay(direction, origin);
	//glm::dvec3 inverseDirection = 1.0 / direction;
	RaycastResult bestResult;
	bestResult.distance = INFINITY;
	bestResult.object = nullptr;
	for (auto& collider: hitColliders) {
		auto result = collider->physicsMesh->Raycast(origin, direction, collider->object);
		if (result.object && result.distance < bestResult.distance) {
			bestResult = result;
		}
	}
	if (bestResult.object) {
		bestResult.distance = glm::sqrt(bestResult.distance);
	}
	return bestResult;
}
