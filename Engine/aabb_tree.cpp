#include "aabb_tree.hpp"

AABBTree<Collider*>& GameobjectSAS() {
    static AABBTree<Collider*> sas;
    return sas;
}
