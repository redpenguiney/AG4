#pragma once
#include <memory>
#include <vector>
#include <array>
#include "aabb.hpp"
#include "assert.hpp"

class Collider;

// A 27-tree (icoseptre) based spatial acceleration structure.
template <typename StoredT>
class AABBTree {
public:
	constexpr static inline size_t NODE_SPLIT_THRESHOLD = 30;

	AABBTree();
	~AABBTree();

	struct Node {
		AABB bounds;
		glm::dvec3 splitPoint;
		std::vector<StoredT> stored;
		std::array<std::unique_ptr<Node>, 27> children; // some may be nullptr
		bool empty = true;
		bool split = false;
	};

	// Returns pointer to where value is now stored.
	Node* Insert(StoredT value, const AABB& objectBounds) {

	}

	void TrySplitNode(Node* n) {
		Assert(n);
		if (n->stored.size() < NODE_SPLIT_THRESHOLD) return; // don't split if there aren't a lot of objects in this node
		if (n->split == true) return; // we already tried to split and failed

		// decide where to split the node
		// todo: using median might be better
		glm::dvec3 meanPosition = { 0, 0, 0 };
		for (auto& obj : n->stored) {
			meanPosition += obj->aabb.Center();
		}
		splitPoint = meanPosition / (double)objects.size();

		// create child nodes
		for (auto& obj : n->stored) {
			int index = 0;
		}
	}
};

// Fetches the root of the AABBTree
AABBTree<Collider*>& GameobjectSAS();

template<typename StoredT>
inline Node* AABBTree<StoredT>::Insert(StoredT value, const AABB& objectBounds) {
	
	auto retVal = this;
	
	if (empty) {
		empty = false;
		stored.push_back(value);
		bounds = objectBounds;
	}
	if ((objectBounds.min.x < splitPoint.x && objectBounds.max.x > splitPoint.x) || (objectBounds.min.y < splitPoint.y && objectBounds.max.y > splitPoint.y) || (objectBounds.min.z < splitPoint.z && objectBounds.max.z > splitPoint.z)) {
		stored.push_back(value);
		bounds.Grow(objectBounds);
		return this;
	}
	else {
		unsigned storageIndex = 0;
		if (objectBounds.min.x > splitPoint.x) {
			storageIndex += 4;
		}
		if (objectBounds.min.y > splitPoint.y) {
			storageIndex += 2;
		}
		if (objectBounds.min.z > splitPoint.z) {
			storageIndex += 1;
		}
		auto result = children[storageIndex]->Insert(value, objectBounds);
		bounds.Grow(children[storageIndex]->bounds);
		return result;
	}
}
