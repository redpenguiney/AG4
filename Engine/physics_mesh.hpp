#pragma once
#include <memory>
#include <vector>

class Mesh;

class BasePhysicsGeometry {
public:
	BasePhysicsGeometry() = default;
	virtual ~BasePhysicsGeometry() = default;
};

class ConvexPhysicsGeometry : public BasePhysicsGeometry {
public:

	ConvexPhysicsGeometry() = default;
	virtual ~ConvexPhysicsGeometry() = default;
};

// Singleton because there's only one kind of sphere, and scaling is done on the gameobject level.
class SpherePhysicsGeometry : public ConvexPhysicsGeometry {
public:
	static std::shared_ptr<SpherePhysicsGeometry> Get();
};

// Standard issue collider for cubes/anything convex that has no curves
class ConvexMeshPhysicsGeometry : public ConvexPhysicsGeometry {
public:
	static std::shared_ptr<ConvexMeshPhysicsGeometry> FromMesh(const std::shared_ptr<Mesh>& m);

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