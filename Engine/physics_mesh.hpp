#pragma once
#include <memory>
#include <vector>

class BasePhysicsMesh {
public:
	BasePhysicsMesh() = default;
	virtual ~BasePhysicsMesh() = default;
};

class ConvexPhysicsMesh : public BasePhysicsMesh {
public:
	ConvexPhysicsMesh() = default;
	virtual ~ConvexPhysicsMesh() = default;
};

// Singleton because there's only one kind of sphere, and scaling is done on the gameobject level.
class SpherePhysicsMesh : public ConvexPhysicsMesh {
public:
	std::shared_ptr<SpherePhysicsMesh> Get();
};

// Standard issue collider for cubes/anything convex that has no curves
class ConvexMeshCollider : public ConvexPhysicsMesh {
public:

};

// Collisions can be done precisely betwen the boundary of a concave mesh and a convex physics mesh via many triangle-convex collisions.
// Triangles are stored in a spatial acceleration structure.
class ConcaveMeshCollider : public BasePhysicsMesh {
public:

};

//// A collider made up of multiple convex colliders
class ConvexMeshDecompositionCollider : public BasePhysicsMesh {
	std::vector<std::shared_ptr<ConvexMeshCollider>> colliders;

};