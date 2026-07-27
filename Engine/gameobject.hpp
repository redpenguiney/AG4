#pragma once
#include <memory>

#include <glm/vec3.hpp>
#include "glm/mat4x4.hpp"
#include "glm/ext/quaternion_float.hpp"
#include "graphics_engine.hpp"
#include "memory_pool.hpp"
#include "collider.hpp"
#include <animation.hpp>
#include "event.hpp"
#include <collision_detection.hpp>
#include "component.hpp"

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

	Collider* const GetCollider() const;
	glm::vec3 ObjectNormalToWorld(glm::vec3 objectNormal);
	glm::vec3 WorldNormalToObject(glm::vec3 worldNormal);

	float elasticity;
	float friction;

	std::shared_ptr<Mesh> GetMesh();

	void SetBoneTransform(unsigned index, glm::mat4x4 transform);

	std::tuple<glm::dvec3, glm::quat, glm::vec3> GetBoneWorldTransform(unsigned index);
	// animation needs to be from the mesh used by ths gameobject. obviously.
	void PlayAnimation(const Animation& anim);

	// fired immediately at the end of the constructor. do not dynamic_cast the gameobject.
	static inline Event<Gameobject>& onGameobjectCreated = Event<Gameobject>::New();

	// fired immediately at the beginning of the destructor, gameobject will still be valid when this is called. do not dynamic_cast the gameobject.
	static inline Event<Gameobject>& onGameobjectDestroyed = Event<Gameobject>::New();

	// ignores layers and Collider::canCollide
	// both this object and the other object must have colliders
	std::optional<Collision> TestCollision(Gameobject* other);

	// returns nullptr if the component does not exist. returns first instance of created component.
	// components are owned by the gameobject they are attached to and will be destroyed with the gameobject; don't hold on to component pointers past gameobject lifetime.
	template<ComponentType T>
	T* GetComponent() {
		for (auto& c : components->components) {
			auto cast = dynamic_cast<T*>(c.get());
			if (cast) return cast;
		}
		return nullptr;
	}

	// returns pointer to created component. you can add multiple of the same component if you want.
	// components are owned by the gameobject they are attached to and will be destroyed with the gameobject; don't hold on to component pointers past gameobject lifetime.
	template <ComponentType T, typename... ConstructorArgs>
	T* AddComponent(ConstructorArgs... compArgs) {
		if (!components) components = std::make_unique<GameobjectComponents>();
		auto ptr = new T(this, compArgs...);
		components->components.emplace_back(ptr);
		return ptr;
	}

	// does nothing if the component does not exist. erases first instance of component type
	template <ComponentType T>
	void EraseComponent() {
		for (size_t i = 0; i < components->components.size(); i++) {
			if (dynamic_cast<T*>(components->components[i]) != nullptr) {
				components->components[i] = components->components.back();
				components->components.pop_back();
				return;
			}
		}
	}
	
	// does nothing if the component does not exist. erases any instances of component type
	template <ComponentType T>
	void EraseComponents() {
		for (size_t i = 0; i < components->components.size(); i++) {
			if (dynamic_cast<T*>(components->components[i]) != nullptr) {
				components->components[i] = components->components.back();
				components->components.pop_back();
				i--; // underflow is ok here
			}
		}
	}
	
	// returns all instances of component type.
	// components are owned by the gameobject they are attached to and will be destroyed with the gameobject; don't hold on to component pointers past gameobject lifetime.
	template <ComponentType T>
	std::vector<T*> GetComponents() {
		std::vector<T*> ret;
		for (auto& c : components->components) {
			auto cast = dynamic_cast<T*>(c.get());
			if (cast) ret.push_back(cast);
		}
		return ret;
	}

protected:
	static inline std::unordered_map<Gameobject*, Skeleton> skeletons; // only contains gameobjects with skeletons.

	Gameobject(const GameobjectCreateParams& params);
	Gameobject(const Gameobject&) = delete;

	glm::dvec3 position;
	glm::vec3 scale;
	glm::quat rotation;
	glm::mat3x3 rotSclMatrix;
	bool rotSclDirty; // indicates that the variable rotSclMatrix must be recalculated before being used
	bool normalMatDirty; // indicates that the normal matrix must be uploaded to the graphics engine again. todo: could hold false for gameobjects with meshes that don't use normal matrix 
	// Used by MemoryPool. Not the first member in order to A. exploit otherwise wasted padding bytes and B. avoid interfering with free list
	bool live;
	// indicates that the object's AABB in the AABB tree is outdated and must be updated.
	//bool aabbDirty;

	
	unsigned drawInstanceIndex; // undefined if not being drawn
	Meshpool* meshpool; // nullptr if not being drawn
	RenderGroup* renderGroup; // undefined if not being drawn

	// may be nullptr if no collisions
	std::unique_ptr<Collider> collider;

	std::unique_ptr<GameobjectComponents> components;

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

	// torqueAxis is not normalized, length describes amount of torque.
	// returns pair<inverse reduced mass, inverse moi around axis>
	std::pair<float, float> GetInverseReducedMass(glm::vec3 torqueAxis);

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
	friend struct StaticCollisionConstraint;
	friend struct DynamicCollisionConstraint;
	friend struct PBDHelpers;
private:
	void UpdateMass();
};