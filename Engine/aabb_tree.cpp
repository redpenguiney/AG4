#include "aabb_tree.hpp"
#include "collider.hpp"

AABBTree& GameobjectSAS() {
    static AABBTree sas;
    return sas;
}

void AABBTree::OptimizeTree() {
	if (root.dirty) OptimizeDirtyNode(&root);
}

void AABBTree::Insert(Collider* value) {
	value->UpdateAABB();
	const AABB& aabb = value->aabb;

	Node* currentNode = &root;
	while (true) {
		currentNode->bounds.Grow(aabb);

		size_t index = InsertHeuristic(aabb, *currentNode);
		if (index == 27) {
			value->node = currentNode;
			currentNode->stored.push_back(value);
			break;
		}
		else {
			currentNode = currentNode->children[index].get();
		}
	}
}

void AABBTree::UpdatePosition(Collider* value) {
	// Go up the tree from the collider's current node to find the first node that fully envelopes the collider.
	// (if collider's current node still envelops the collider, this will do nothing)
	Node* oldNode = value->node;
	Node* currentNode = oldNode;
	while (currentNode->parent) {
		if (currentNode->bounds.TestEnvelopes(value->aabb)) {
			break;
		}
		currentNode = currentNode->parent;
	}

	// find child to insert node into
	while (true) {
		currentNode->bounds.Grow(value->aabb);
		size_t index = InsertHeuristic(value->aabb, *currentNode);
		if (index == 27) {
			break;
		}
		else {
			currentNode = currentNode->children[index].get();
		}
	}

	if (oldNode != currentNode) {
		// remove object from prior node
		for (unsigned i = 0; i < oldNode->stored.size(); i++) {
			if (oldNode->stored[i] == value) {
				oldNode->stored[i] = oldNode->stored.back();
				oldNode->stored.pop_back();
				break;
			}
		}

		// add object to new node
		value->node = currentNode;
		currentNode->stored.push_back(value);
	}
}

void AABBTree::Remove(Collider* value) {
	Node* currentNode = value->node;

	// remove object from node
	for (unsigned i = 0; i < currentNode->stored.size(); i++) {
		if (currentNode->stored[i] == value) {
			currentNode->stored[i] = currentNode->stored.back();
			currentNode->stored.pop_back();
			break;
		}
	}
	
	// mark the node and its ancestry for resizing/trimming if neccesary
	while (!currentNode->dirty) {
		currentNode->dirty = true;
		currentNode = currentNode->parent;
	}

	value->node = nullptr;
}

void AABBTree::OptimizeDirtyNode(Node* n) {
	
}

void AABBTree::TrySplitNode(AABBTree::Node* n) {
	Assert(n);
	if (n->stored.size() < NODE_SPLIT_THRESHOLD) return; // don't split if there aren't a lot of objects in this node
	if (n->split == true) return; // we already tried to split and failed
	n->split = true;

	// decide where to split the node
	// todo: using median might be better
	glm::dvec3 meanPosition = { 0, 0, 0 };
	for (auto& obj : n->stored) {
		meanPosition += obj->aabb.Center();
	}
	n->splitPoint = meanPosition / (double)n->stored.size();

	// Confirm that not all objects are going into the same child. (most likely the pathological case where all the objects are at the same position)
	// If that's the case, we'd just keep pointlessly splitting this node's children and kill performance if we didn't check for it.
	size_t selectedIndex = 999;
	for (auto& obj : n->stored) {
		size_t index = InsertHeuristic(obj->aabb, *n);
		if (selectedIndex == 999) {
			selectedIndex = index;
		}
		else if (index != selectedIndex) {
			goto weCanSplit;
		}
	}
	return; // if we didn't do the goto, then we can't split the node.
	
	weCanSplit:;
	for (unsigned i = 0; i < n->stored.size(); i ++) {
		auto obj = n->stored[i];
		size_t index = InsertHeuristic(obj->aabb, *n);
		if (index == 27) {
			continue; // we keep the object in what is now an interior node
		}
		else {
			if (n->children[index] == nullptr) {
				n->children[index] = std::make_unique<Node>();
				n->children[index]->parent = n;
				n->children[index]->bounds = obj->aabb;
				//n->children[index]->empty = false;
				n->children[index]->split = false;
			}
			else {
				n->children[index]->bounds.Grow(obj->aabb);
			}
			n->children[index]->stored.push_back(obj);
			obj->node = n->children[index].get();
			n->stored[i] = n->stored.back();
			n->stored.pop_back();
			i--;
		}
	}
}

size_t AABBTree::InsertHeuristic(const AABB& store, const Node& node) {
	if (!node.split) { // then this is a leaf
		return 27;
	}
	if (store.Volume() * 0.5 >= node.bounds.Volume()) return 27;
	size_t index = 0;
	if (store.max.x > node.splitPoint.x) {
		index += 9;
		if (store.min.x > node.splitPoint.x)
			index += 9;
	}
	if (store.max.y > node.splitPoint.y) {
		index += 3;
		if (store.min.y > node.splitPoint.y)
			index += 3;
	}
	if (store.max.z > node.splitPoint.z) {
		index += 1;
		if (store.min.z > node.splitPoint.z)
			index += 1;
	}
	return index;
}
