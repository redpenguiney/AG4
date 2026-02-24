#pragma once
#include <memory>
#include <vector>
#include <array>
#include "aabb.hpp"
#include "assert.hpp"

class Collider;

// A 27-tree (icoseptre) based spatial acceleration structure.
class AABBTree {
public:
	constexpr static inline size_t NODE_SPLIT_THRESHOLD = 30;

	AABBTree();
	~AABBTree();

	struct Node {
		Node* parent;

		AABB bounds;
		glm::dvec3 splitPoint;
		// each collider stores a reference to the node it's stored in, be careful
		std::vector<Collider*> stored;
		std::array<std::unique_ptr<Node>, 27> children; // some may be nullptr
		bool empty = true;
		bool split = false;
	};

	// Sets value::node to where value is now stored.
	void Insert(Collider* value);

	void Remove(Collider* value);

private:

	Node root;

	void TrySplitNode(Node* n);

	// returns index into children, or 27 if should be stored in the given node (either because it's a leaf or because it's big enough to go in an interior node)
	size_t InsertHeuristic(const AABB& store, const Node& node);
};


AABBTree& GameobjectSAS();