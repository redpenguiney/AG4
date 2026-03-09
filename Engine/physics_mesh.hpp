#pragma once
#include <memory>
#include <vector>
#include <array>
#include "glm/vec3.hpp"
#include "glm/mat3x3.hpp"
#include "raycast.hpp"

class Mesh;
class Gameobject;

class BasePhysicsGeometry {
public:
	BasePhysicsGeometry() = default;
	virtual ~BasePhysicsGeometry() = default;

	// todo: very expensive function. cache so that if multiple rigidbodies with same BasePhysicsGeometry and size are made, they can reuse the same calculation
	glm::mat3x3 GetMomentOfInertia(glm::vec3 objectScale, float objectMass);

	// Returned distance is actually distance squared
	virtual RaycastResult Raycast(glm::dvec3 origin, glm::dvec3 direction, Gameobject* object) = 0;

	virtual void AddLocalMomentOfInertiaContribution(glm::vec3& centerOfMass, float& Ia, float& Ib, float& Ic, float& Iap, float& Ibp, float& Icp, glm::vec3 objectScale, float density) = 0;
	virtual float Volume(glm::vec3 objectScale) = 0;
};

class ConvexPhysicsGeometry : public BasePhysicsGeometry {
public:
	virtual RaycastResult Raycast(glm::dvec3 origin, glm::dvec3 direction, Gameobject* object) = 0;

	// returns furthest point on surface in that direction, used for collision detection
	virtual glm::vec3 Support(glm::vec3 direction) const = 0;
	ConvexPhysicsGeometry() = default;
	virtual ~ConvexPhysicsGeometry() = default;

	virtual void AddLocalMomentOfInertiaContribution(glm::vec3& centerOfMass, float& Ia, float& Ib, float& Ic, float& Iap, float& Ibp, float& Icp, glm::vec3 objectScale, float density) = 0;
	virtual float Volume(glm::vec3 objectScale) = 0;
};

// Singleton because there's only one kind of sphere, and scaling is done on the gameobject level.
// Doesn't support non-uniform scaling because then it's not a sphere.
// radius is 0.5f
class SpherePhysicsGeometry : public ConvexPhysicsGeometry {
public:
	static std::shared_ptr<SpherePhysicsGeometry> Get();
	
	RaycastResult Raycast(glm::dvec3 origin, glm::dvec3 direction, Gameobject* object) override;

	virtual glm::vec3 Support(glm::vec3 direction) const override;
	virtual void AddLocalMomentOfInertiaContribution(glm::vec3& centerOfMass, float& Ia, float& Ib, float& Ic, float& Iap, float& Ibp, float& Icp, glm::vec3 objectScale, float density) override;
	virtual float Volume(glm::vec3 objectScale) override;
private:
	SpherePhysicsGeometry();
};

// Standard issue collider for cubes/anything convex that has no curves
// TODO: version with octree or something for faster raycasting
class ConvexMeshPhysicsGeometry : public ConvexPhysicsGeometry {
public:
	RaycastResult Raycast(glm::dvec3 origin, glm::dvec3 direction, Gameobject* object) override;

	const std::vector<std::array<glm::vec3, 3>> triangles;

	// vertices are divided into 8 quadrants
	// index from searchDirection: +4 if nonnegative x, +2 if nonnegative y, +1 in nonnegative z 
	// this structure makes support function 8 times faster
	const std::array<std::vector<glm::vec3>, 8> supportVertices;
	static std::shared_ptr<ConvexMeshPhysicsGeometry> FromMesh(const std::shared_ptr<Mesh>& m);

	virtual float Volume(glm::vec3 objectScale) override;
	virtual glm::vec3 Support(glm::vec3 direction) const override;
	virtual void AddLocalMomentOfInertiaContribution(glm::vec3& centerOfMass, float& Ia, float& Ib, float& Ic, float& Iap, float& Ibp, float& Icp, glm::vec3 objectScale, float density) override;

private:
	ConvexMeshPhysicsGeometry(std::vector<std::array<glm::vec3, 3>> tris, std::array<std::vector<glm::vec3>, 8> supportVerts);
};

// Collisions can be done precisely betwen the boundary of a concave mesh and a convex physics mesh via many triangle-convex collisions.
// Triangles are stored in a spatial acceleration structure.
class ConcaveMeshPhysicsCollider : public BasePhysicsGeometry {
public:

};

//// A collider made up of multiple convex colliders
class ConvexMeshDecompositionPhysicsGeometry : public BasePhysicsGeometry {
	std::vector<std::shared_ptr<ConvexMeshPhysicsGeometry>> colliders;

};