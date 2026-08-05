#pragma once
#include <memory>
#include <vector>
#include <array>
#include "aabb.hpp"
#include "assert.hpp"
#include <bitset>
#include "collision_layers.hpp"

//#define DEBUG_AABBTREE_VISUALIZATION 1

class Collider;
class Gameobject;

// A 27-tree (icoseptre) based spatial acceleration structure.
class AABBTree {
public:
	constexpr static inline size_t NODE_SPLIT_THRESHOLD = 30;

	AABBTree();
	~AABBTree();

	struct Node {
		Node* parent = nullptr;
		AABB bounds;
		glm::dvec3 splitPoint;
		// each collider stores a reference to the node it's stored in, be careful
		std::vector<Collider*> stored;
		std::array<std::unique_ptr<Node>, 27> children; // some may be nullptr
		// true if bounds/splitPoint are undefined
		bool empty = true;
		bool split = false;
		// indicates that the node's bounding box is bigger than it needs to be and should be recalculated
		bool dirty = false; 
#ifdef  DEBUG_AABBTREE_VISUALIZATION
		std::unique_ptr<Gameobject> visualizer;
#endif //  DEBUG_AABBTREE_VISUALIZATION


	};


	// Returns list of colliders whose AABBs intersect the given ray.
	std::vector<Collider*> QueryRay(glm::dvec3 direction, glm::dvec3 origin);

	// Returns list of colliders whose AABBs intersect the given AABB.
	std::vector<Collider*> QueryAABB(const AABB& aabb);

	// call every frame so that removed objects don't leave the tree unoptimized
	void OptimizeTree();

	// Sets value::node to where value is now stored.
	void Insert(Collider* value);

	// AABB of collider should be updated before you call this.
	void UpdatePosition(Collider* value);
	
	void Remove(Collider* value);

private:
	void CollectRayIntersections(std::vector<Collider*>& list, Node* n, const glm::dvec3 inverseDirection, const glm::dvec3 origin);
	void CollectAABBIntersections(std::vector<Collider*>& list, Node* n, const AABB& aabb);

	std::unique_ptr<Node> CreateChildNode(Node* parent, Collider* obj);

	// returns true if the node is empty and can be removed
	bool OptimizeDirtyNode(Node* n);

	Node root;

	void TrySplitNode(Node* n);

	// returns index into children, or 27 if should be stored in the given node (either because it's a leaf or because it's big enough to go in an interior node)
	size_t InsertHeuristic(const AABB& store, const Node& node);
};


AABBTree& GameobjectSAS();