#ifndef QUAD_TREE_H
#define QUAD_TREE_H
#include <vector>
#include <memory>

#include "gpr925/physics_world.h"
#include "gpr925/vec2.h"
#include "vector2.h"

namespace gpr925 {

class Rectangle {
public:
    Rectangle(Vec2 position, int width, int height) : _position(position), _width(width), _height(height) {}
    bool contains(PhysicsBody& point);
    bool intersects(Rectangle& range);
    Vec2& getPosition() { return _position; }
    int getWidth() { return _width; }
    int getHeight() { return _height; } 
private:
    Vec2 _position;
    int _width = 0;
    int _height = 0;
};

class QuadTreeNode{
public:
    QuadTreeNode(Rectangle& boundary, size_t count) : _boundary(boundary), _count(count) {}
    bool insert(PhysicsBody& point);
    std::vector<PhysicsBody> query(Rectangle& range, std::vector<PhysicsBody>& found);
    void subdivide();
    bool isLeaf();
    void clear();
    Rectangle& getBoundary() { return _boundary; }
    std::vector<std::unique_ptr<QuadTreeNode>>& getChildren() { return _children; }

private:
    std::vector<std::unique_ptr<QuadTreeNode>> _children;
    Rectangle _boundary;
    std::vector<PhysicsBody> _points;
    size_t _count;
    bool _divided = false;
};

class QuadTree {
public:
    QuadTree(Rectangle& boundary, size_t capacity) : _root(std::make_unique<QuadTreeNode>(boundary, capacity)), _count(0) {}
    bool insert(PhysicsBody& point);
    std::vector<PhysicsBody> query(Rectangle& range, std::vector<PhysicsBody>& found);
    void clear();
    void ReBuild(Rectangle& newBoundary, size_t newCount);

    std::unique_ptr<QuadTreeNode>& getRoot() { return _root; }

private:
    std::unique_ptr<QuadTreeNode> _root;
    size_t _count = 0;
};

} // namespace gpr925

#endif // QUAD_TREE_H