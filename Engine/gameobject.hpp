#pragma once
#include <memory>

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/ext/quaternion_float.hpp"

#include "memory_pool.hpp"

class Collider;
struct DrawHandle {
	unsigned pool; // 0 for null draw handle
	unsigned instanceIndex;
	unsigned meshIndex;
	unsigned drawCommandIndex;
};

class Gameobject {
public:
	// We use a factory function so gameobjects are always created in an object pool.
	// You own the returned pointer and may safely place it in whatever wrappers (std::shared_ptr, std::unique_ptr, etc.) you please.
	static Gameobject* New();

	~Gameobject();

	// Returns this object's memory to the object pool it came from. 
	static void operator delete(void* obj);

	const glm::dvec3& Position() const;
	const glm::vec3& Scale() const;
	const glm::quat& Rotation() const;


protected:

	Gameobject();
	Gameobject(const Gameobject&) = delete;

	glm::dvec3 position;
	glm::vec3 scale;
	glm::quat rotation;
	glm::mat3x3 rotSclMatrix;
	bool rotSclDirty;
	// Used by MemoryPool. Not the first member in order to A. exploit otherwise wasted padding bytes and B. avoid interfering with free list
	bool live;

	Collider* collider;
	DrawHandle render;

	friend class MemoryPool<Gameobject>;

};

class Physobject : public Gameobject {
public:
	static Physobject* New();

	static void operator delete(void* obj);

protected:

	Physobject();

	glm::dvec3 lastPos;
	glm::dvec3 nextPos;
	glm::quat lastRot;
	glm::quat nextRot;

	float inverseMass;
	glm::mat3x3 inverseInertiaTensor; // the moment of inertia is like mass, but for rotation.

	float elasticity;
	float friction;

	friend class MemoryPool<Physobject>;

};