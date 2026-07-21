#pragma once
#include <memory>
#include "physics_mesh.hpp"
#include "aabb_tree.hpp"
#include <bitset>
#include "collision_layers.hpp"

class Gameobject;

// Stores the collision data for a specific gameobject, namely its PhysicsMesh and its spatial acceleration structure data.
class Collider {
public:
	Collider(std::shared_ptr<BasePhysicsGeometry> m, Gameobject* obj);
	~Collider();
	const std::shared_ptr<BasePhysicsGeometry> physicsMesh;
	Gameobject* object;

	// the layers this collider collides with in physics simulation. Set to all 0s for no collisions.
	//std::bitset<NUM_COLLISION_LAYERS> layerMask = 1u;

	// the layers this collider belongs to.
	//std::bitset<NUM_COLLISION_LAYERS> layers = 0;

	// the layer this collider belongs to.
	uint8_t layer = 0;

	// disables physics engine collisions while still allowing raycasting/etc.
	bool canCollide = true;

private:
	friend class Gameobject;
	friend class AABBTree;
	friend class PhysicsEngine;

	AABBTree::Node* node; // non-owning
	AABB aabb;
	void UpdateAABB();
};

