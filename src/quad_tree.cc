#include "gpr925/quad_tree.h"

namespace gpr925 {
    #pragma region QuadTree
    bool QuadTree::insert(PhysicsBody& point) {
        if (!_root->insert(point)) {
            return false;
        }
        return false;
    }

    std::vector<PhysicsBody> QuadTree::query(Rectangle& range, std::vector<PhysicsBody>& found) {
        range.intersects(_root->getBoundary());
        return found;
    }

    void QuadTree::clear() {

    }
    #pragma endregion

    #pragma region QuadTreeNode
    bool QuadTreeNode::insert(PhysicsBody& point) {
        if (!_boundary.contains(point)) {
            return false;
        }
        return false;
    }

    std::vector<PhysicsBody> QuadTreeNode::query(Rectangle& range, std::vector<PhysicsBody>& found) {
        range.intersects(_boundary);
        return found;
    }

    void QuadTreeNode::subdivide() {}
    bool QuadTreeNode::isLeaf() { return false; }
    #pragma endregion

    #pragma region Rectangle
    bool Rectangle::contains(PhysicsBody& point) {
        if (point.position.x() >= 1) return true;
        return false; 
    }
    bool Rectangle::intersects(Rectangle& range) {
        if (range._height >= 1) return true; 
        return false; 
    }
    #pragma endregion

    


    
}
