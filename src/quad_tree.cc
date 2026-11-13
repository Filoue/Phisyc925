#include "gpr925/quad_tree.h"

namespace gpr925 {
    Scalar S(int value) {
    return static_cast<Scalar>(value);
    }

    #pragma region QuadTree
    bool QuadTree::insert(PhysicsBody& point) {
        return _root->insert(point);
    }


    std::vector<PhysicsBody> QuadTree::query(Rectangle& range, std::vector<PhysicsBody>& found){
        return _root->query(range, found);
    }

    void QuadTree::clear() {
        _root->clear();
    }

    void QuadTree::ReBuild(Rectangle& newBoundary, size_t newCount) {
        _root = std::make_unique<QuadTreeNode>(newBoundary, newCount);
    }
    #pragma endregion

    #pragma region QuadTreeNode
    bool QuadTreeNode::insert(PhysicsBody& point) {
        if (!_boundary.contains(point)) {
            return false;
        }

        if (_points.size() < _count) {
            _points.push_back(point);
            return true;
        } else {
            if (!_divided) {
                subdivide();
            }
            for (auto& child : _children) {
                if (child->insert(point)) {
                    return true;
                }
            }
        }
        return false;
    }

    std::vector<PhysicsBody> QuadTreeNode::query(Rectangle& range, std::vector<PhysicsBody>& found) {
        if (range.intersects(_boundary)) {
            for (auto& point : _points) {
                if (range.contains(point)) {
                    found.push_back(point);
                }
            }
            if (_divided) {
                for (const auto& child : _children) {
                    child->query(range, found);
                }
            }
        }
        return found;
    }

    void QuadTreeNode::subdivide() {
        Vec2 pos = _boundary.getPosition();
        int w = _boundary.getWidth() / 2;
        int h = _boundary.getHeight() / 2;

        if (_divided) {
            return; // Already subdivided, do nothing
        }

        Rectangle ne(Vec2(pos.x() + S(w), pos.y()), w, h);
        Rectangle nw(Vec2(pos.x(), pos.y()), w, h);
        Rectangle se(Vec2(pos.x() + S(w), pos.y() + S(h)), w, h);
        Rectangle sw(Vec2(pos.x(), pos.y() + S(h)), w, h);

        _children.push_back(std::make_unique<QuadTreeNode>(ne, _count));
        _children.push_back(std::make_unique<QuadTreeNode>(nw, _count));
        _children.push_back(std::make_unique<QuadTreeNode>(se, _count));
        _children.push_back(std::make_unique<QuadTreeNode>(sw, _count));

        _divided = true;
    }
    bool QuadTreeNode::isLeaf() {
        return !_divided;
    }

    void QuadTreeNode::clear() {
        _points.clear();
        for (auto& child : _children) {
            child->clear();
        }
        _children.clear();
        _divided = false;
    }
    #pragma endregion

    #pragma region Rectangle
    bool Rectangle::contains(PhysicsBody& point) {
        Vec2 pos = point.position;
        return (pos.x() >= _position.x() && pos.x() <= _position.x() + S(_width) &&
                pos.y() >= _position.y() && pos.y() <= _position.y() + S(_height));
    }

    bool Rectangle::intersects(Rectangle& range)  {
        return !(_position.x() + S(_width) < range.getPosition().x() ||
                 _position.x() > range.getPosition().x() + S(range.getWidth()) ||
                 _position.y() + S(_height) < range.getPosition().y() ||
                 _position.y() > range.getPosition().y() + S(range.getHeight()));
    }
    #pragma endregion

    


    
}
