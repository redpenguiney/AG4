#pragma once
#include <memory>
#include "physics_mesh.hpp"

class Gameobject;

// Stores the collision data for a specific gameobject, namely its PhysicsMesh and its spatial acceleration structure data.
class Collider {
public:
	Collider();
	~Collider();
	const std::shared_ptr<BasePhysicsMesh> physicsMesh;
	Gameobject* object;
private:
};

