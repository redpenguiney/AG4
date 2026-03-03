#pragma once
#include <memory>
#include "physics_mesh.hpp"
#include "aabb_tree.hpp"
#include <bitset>

class Gameobject;

// Stores the collision data for a specific gameobject, namely its PhysicsMesh and its spatial acceleration structure data.
class Collider {
public:
	Collider(std::shared_ptr<BasePhysicsGeometry> m, Gameobject* obj);
	~Collider();
	const std::shared_ptr<BasePhysicsGeometry> physicsMesh;
	Gameobject* object;
	std::bitset<32> collisionLayers = 1u;
private:
	friend class Gameobject;
	friend class AABBTree;
	friend class PhysicsEngine;

	AABBTree::Node* node; // non-owning
	AABB aabb;
	void UpdateAABB();
};

