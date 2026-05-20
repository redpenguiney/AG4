#pragma once
#include <memory>

#include <glm/vec3.hpp>
#include "glm/mat4x4.hpp"
#include "glm/ext/quaternion_float.hpp"
#include "graphics_engine.hpp"
#include "memory_pool.hpp"
#include "collider.hpp"
#include <animation.hpp>

class Mesh;
class RenderPass;
class RenderGroup;
class Meshpool;
struct VertexAttribute;
union VertexScalar;

struct GameobjectCreateParams {
	std::vector<std::shared_ptr<DrawPass>> renderPasses = GraphicsEngine::Get().defaultDrawingPasses;
	
	GLenum primitiveType = GL_TRIANGLES;

	// if not provided, the gameobject won't be rendered
	std::shared_ptr<Mesh> mesh;	

	// if not provided, the gameobject won't have collisions
	std::shared_ptr<BasePhysicsGeometry> physicsMesh;
};

class Gameobject {
public:
	// We use a factory function so gameobjects are always created in an object pool.
	// You own the returned pointer and may safely place it in whatever wrappers (std::shared_ptr, std::unique_ptr, etc.) you please.
	static Gameobject* New(const GameobjectCreateParams& params);

	virtual ~Gameobject();

	// Returns this object's memory to the object pool it came from. 
	static void operator delete(void* obj);

	const glm::dvec3& Position() const;
	const glm::vec3& Scale() const;
	const glm::quat& Rotation() const;

	void SetPosition(const glm::dvec3&);
	virtual void SetScale(const glm::vec3&);
	void SetRotation(const glm::quat&);

	// attrib and value better match up, or you'll be lucky if you get a segfault
	void SetInstanceAttribute(const VertexAttribute& attrib, VertexScalar* value);

	void SetInstanceAttribute(const VertexAttribute& attrib, glm::vec4 value);
	void SetInstanceAttribute(const VertexAttribute& attrib, float value);


	const glm::mat3x3& GetRotSclMatrix();

	bool Live();

	using Pool = MemoryPool<Gameobject, const GameobjectCreateParams&>;

	const Collider* const GetCollider() const;
	glm::vec3 ObjectNormalToWorld(glm::vec3 objectNormal);
	glm::vec3 WorldNormalToObject(glm::vec3 worldNormal);

	float elasticity;
	float friction;

	std::shared_ptr<Mesh> GetMesh() const;

protected:

	Gameobject(const GameobjectCreateParams& params);
	Gameobject(const Gameobject&) = delete;

	glm::dvec3 position;
	glm::vec3 scale;
	glm::quat rotation;
	glm::mat3x3 rotSclMatrix;
	bool rotSclDirty; // indicates that the variable rotSclMatrix must be recalculated before being used
	bool normalMatDirty; // indicates that the normal matrix must be uploaded to the graphics engine again. TODO could hold false for gameobjects with meshes that don't use normal matrix 
	// Used by MemoryPool. Not the first member in order to A. exploit otherwise wasted padding bytes and B. avoid interfering with free list
	bool live;
	// indicates that the object's AABB in the AABB tree is outdated and must be updated.
	//bool aabbDirty;
	
	unsigned drawInstanceIndex; // undefined if not being drawn
	Meshpool* meshpool; // nullptr if not being drawn
	RenderGroup* renderGroup; // undefined if not being drawn
	Skeleton skeleton; // todo: move elsewhere

	// may be nullptr if no collisions
	std::unique_ptr<Collider> collider;

	friend class Pool;
	friend class RenderGroup;
	friend class GraphicsEngine;
	friend class PhysicsEngine; 

private:

};

using PhysobjectCreateParams = GameobjectCreateParams;

class Physobject : public Gameobject {
public:
	static Physobject* New(const PhysobjectCreateParams& params);

	static void operator delete(void* obj);

	virtual ~Physobject();

	using Pool = MemoryPool<Physobject, const PhysobjectCreateParams&>;

	virtual void SetScale(const glm::vec3&) override;

	// pass INFINITY to make object immobile
	void SetMass(float mass);

	void SetMassFromDensity(float density);

	glm::vec3 velocity;
	glm::vec3 rotVelocity;

protected:

	Physobject(const PhysobjectCreateParams& params);

	glm::dvec3 lastPos;
	glm::dvec3 nextPos;
	union {
		glm::quat lastRot;
		glm::vec3 nextRotVel;
	};
	union {
		glm::quat nextRot;
		glm::vec3 nextVel;
	};

	glm::mat3x3 inverseInertiaTensor; // the moment of inertia is like mass, but for rotation.
	
	float inverseMass;

	friend class Pool;
	friend class PhysicsEngine;
	friend class StaticCollisionConstraint;
	friend class DynamicCollisionConstraint;
private:
	void UpdateMass();
};