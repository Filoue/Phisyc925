#include <gtest/gtest.h>

#include <algorithm>
#include <vector>
#include <cstddef>
#include <random>
#include <set>
#include <utility>

#include "vector2.h"
#include "gpr925/quad_tree.h"

namespace gpr925 {
namespace {

using PairSet = std::set<std::pair<size_t, size_t>>;

Scalar S(int value) {
    return static_cast<Scalar>(value);
}

Rectangle createRectangle(int x, int y, int width, int height) {
    return Rectangle(Vec2{S(x), S(y)}, width, height);
}

PairSet createPairSet(const std::vector<PhysicsBody>& points) {
    PairSet pairSet;
    for (const auto& point : points) {
        pairSet.emplace(point.position.x(), point.position.y());
    }
    return pairSet;
}

size_t CountNodes(QuadTreeNode& node) {
    size_t count = 1;  // Count the current node
    for (const auto& child : node.getChildren()) {
        if (child) {
            count += CountNodes(*child);
        }
    }
    return count;
}

TEST(QuadTreeTest, EmptyTreeHasNoPairs) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTree quadTree(boundary, 4);

    Rectangle queryRange = createRectangle(10, 10, 20, 20);
    std::vector<PhysicsBody> foundPoints;
    quadTree.query(queryRange, foundPoints);

    EXPECT_TRUE(foundPoints.empty());
}

TEST(QuadTreeTest, InsertPointsAndQuery) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTree quadTree(boundary, 4);

    // Insert points into the QuadTree
    std::vector<PhysicsBody> pointsToInsert;
    for (int i = 0; i < 10; ++i) {
        PhysicsBody point;
        point.position = Vec2(S(i * 10), S(i * 10));
        pointsToInsert.push_back(point);
        quadTree.insert(point);
    }

    // Query a range that should contain some of the inserted points
    Rectangle queryRange = createRectangle(20, 20, 40, 40);
    std::vector<PhysicsBody> foundPoints;
    quadTree.query(queryRange, foundPoints);

    // Create a set of expected pairs based on the inserted points
    PairSet expectedPairs;
    for (auto& point : pointsToInsert) {
        if (queryRange.contains(point)) {
            expectedPairs.emplace(point.position.x(), point.position.y());
        }
    }

    // Create a set of actual pairs from the found points
    PairSet actualPairs = createPairSet(foundPoints);

    EXPECT_EQ(expectedPairs, actualPairs);
}

TEST(QuadTreeTest, SubdivideAndCountNodes) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTree quadTree(boundary, 4);

    // Insert points to trigger subdivision
    for (int i = 0; i < 20; ++i) {
        PhysicsBody point;
        point.position = Vec2(S(i * 5), S(i * 5));
        quadTree.insert(point);
    }

    // Count the number of nodes in the QuadTree
    size_t nodeCount = CountNodes(*quadTree.getRoot());

    // Expect more than one node due to subdivision
    EXPECT_GT(nodeCount, 1);
}

TEST(QuadTreeTest, ClearRemovesAllPoints) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTree quadTree(boundary, 4);

    // Insert points into the QuadTree
    for (int i = 0; i < 10; ++i) {
        PhysicsBody point;
        point.position = Vec2(S(i * 10), S(i * 10));
        quadTree.insert(point);
    }

    // Clear the QuadTree
    quadTree.clear();

    // Query a range that previously contained points
    Rectangle queryRange = createRectangle(0, 0, 100, 100);
    std::vector<PhysicsBody> foundPoints;
    quadTree.query(queryRange, foundPoints);

    EXPECT_TRUE(foundPoints.empty());
}



}}