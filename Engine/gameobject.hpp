#pragma once
#include <memory>

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/ext/quaternion_float.hpp"

#include "memory_pool.hpp"

class Collider;
class Mesh;
class RenderPass;
class RenderGroup;
class Meshpool;

struct DrawHandle {
	Meshpool* pool; // nullptr for null draw handle
	unsigned instanceIndex;
	RenderGroup* group;
};

struct GameobjectCreateParams {
	std::vector<std::shared_ptr<RenderPass>> renderPasses;
	std::shared_ptr<Mesh> mesh;
};

class Gameobject {
public:
	// We use a factory function so gameobjects are always created in an object pool.
	// You own the returned pointer and may safely place it in whatever wrappers (std::shared_ptr, std::unique_ptr, etc.) you please.
	static Gameobject* New(GameobjectCreateParams params);

	~Gameobject();

	// Returns this object's memory to the object pool it came from. 
	static void operator delete(void* obj);

	const glm::dvec3& Position() const;
	const glm::vec3& Scale() const;
	const glm::quat& Rotation() const;

	void SetPosition(const glm::dvec3&);
	void SetScale(const glm::vec3&);
	void SetRotation(const glm::quat&);

	const glm::mat3x3& GetRotSclMatrix();

	bool Live();

protected:

	Gameobject(GameobjectCreateParams params);
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
	friend class RenderGroup;
	friend class GraphicsEngine;

private:

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