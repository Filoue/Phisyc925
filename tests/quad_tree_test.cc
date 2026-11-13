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

PhysicsBody createPoint(int x, int y) {
    PhysicsBody point;
    point.position = Vec2(S(x), S(y));
    return point;
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

TEST(QuadTreeNodeTest, IsLeafReturnsTrueForLeafNode) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTreeNode node(boundary, 4);

    EXPECT_TRUE(node.isLeaf());
}

TEST(QuadTreeNodeTest, IsLeafReturnsFalseAfterSubdivision) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTreeNode node(boundary, 4);

    // Insert points to trigger subdivision
    for (int i = 0; i < 5; ++i) {
        PhysicsBody point;
        point.position = Vec2(S(i * 10), S(i * 10));
        node.insert(point);
    }

    EXPECT_FALSE(node.isLeaf());
}

TEST(QuadTreeNodeTest, SubdivideCreatesChildren) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTreeNode node(boundary, 4);

    // Insert points to trigger subdivision
    for (int i = 0; i < 5; ++i) {
        PhysicsBody point;
        point.position = Vec2(S(i * 10), S(i * 10));
        node.insert(point);
    }

    node.subdivide();

    // Check that the node has children after subdivision
    EXPECT_FALSE(node.getChildren().empty());
}

TEST(QuadTreeNodeTest, QueryReturnsPointsWithinRange) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTreeNode node(boundary, 4);

    // Insert points into the QuadTreeNode
    std::vector<PhysicsBody> pointsToInsert;
    for (int i = 0; i < 10; ++i) {
        PhysicsBody point;
        point.position = Vec2(S(i * 10), S(i * 10));
        pointsToInsert.push_back(point);
        node.insert(point);
    }

    // Query a range that should contain some of the inserted points
    Rectangle queryRange = createRectangle(20, 20, 40, 40);
    std::vector<PhysicsBody> foundPoints;
    node.query(queryRange, foundPoints);

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

TEST(QuadTreeNodeTest, QueryReturnsEmptyForNoPointsInRange) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTreeNode node(boundary, 4);

    // Insert points into the QuadTreeNode
    for (int i = 0; i < 10; ++i) {
        PhysicsBody point;
        point.position = Vec2(S(i * 10), S(i * 10));
        node.insert(point);
    }

    // Query a range that should not contain any of the inserted points
    Rectangle queryRange = createRectangle(200, 200, 50, 50);
    std::vector<PhysicsBody> foundPoints;
    node.query(queryRange, foundPoints);

    EXPECT_TRUE(foundPoints.empty());
}

TEST(RectangleTest, ContainsReturnsTrueForPointInside) {
    Rectangle rect = createRectangle(0, 0, 100, 100);
    PhysicsBody point;
    point.position = Vec2(S(50), S(50));

    EXPECT_TRUE(rect.contains(point));
}

TEST(RectangleTest, ContainsReturnsFalseForPointOutside) {
    Rectangle rect = createRectangle(0, 0, 100, 100);
    PhysicsBody point;
    point.position = Vec2(S(150), S(150));

    EXPECT_FALSE(rect.contains(point));
}

TEST(RectangleTest, IntersectsReturnsTrueForOverlappingRectangles) {
    Rectangle rect1 = createRectangle(0, 0, 100, 100);
    Rectangle rect2 = createRectangle(50, 50, 100, 100);

    EXPECT_TRUE(rect1.intersects(rect2));
}

TEST(RectangleTest, IntersectsReturnsFalseForNonOverlappingRectangles) {
    Rectangle rect1 = createRectangle(0, 0, 100, 100);
    Rectangle rect2 = createRectangle(150, 150, 100, 100);

    EXPECT_FALSE(rect1.intersects(rect2));
}

TEST(RectangleTest, IntersectsReturnsTrueForTouchingRectangles) {
    Rectangle rect1 = createRectangle(0, 0, 100, 100);
    Rectangle rect2 = createRectangle(100, 100, 50, 50);

    EXPECT_TRUE(rect1.intersects(rect2));
}

TEST(RectangleTest, IntersectsReturnsTrueForContainedRectangles) {
    Rectangle rect1 = createRectangle(0, 0, 100, 100);
    Rectangle rect2 = createRectangle(25, 25, 50, 50);

    EXPECT_TRUE(rect1.intersects(rect2));
}

TEST(RectangleTest, IntersectsReturnsTrueForIdenticalRectangles) {
    Rectangle rect1 = createRectangle(0, 0, 100, 100);
    Rectangle rect2 = createRectangle(0, 0, 100, 100);

    EXPECT_TRUE(rect1.intersects(rect2));
}

TEST(RectangleTest, IntersectsReturnsTrueForTouchingEdges) {
    Rectangle rect1 = createRectangle(0, 0, 100, 100);
    Rectangle rect2 = createRectangle(100, 0, 50, 50);

    EXPECT_TRUE(rect1.intersects(rect2));
}

TEST(RectangleTest, IntersectsReturnsTrueForTouchingCorners) {
    Rectangle rect1 = createRectangle(0, 0, 100, 100);
    Rectangle rect2 = createRectangle(100, 100, 50, 50);

    EXPECT_TRUE(rect1.intersects(rect2));
}

TEST(RectangleTest, IntersectsReturnsTrueForContainedEdges) {
    Rectangle rect1 = createRectangle(0, 0, 100, 100);
    Rectangle rect2 = createRectangle(25, 0, 50, 50);

    EXPECT_TRUE(rect1.intersects(rect2));
}

TEST(RectangleTest, IntersectsReturnsTrueForContainedCorners) {
    Rectangle rect1 = createRectangle(0, 0, 100, 100);
    Rectangle rect2 = createRectangle(0, 25, 50, 50);

    EXPECT_TRUE(rect1.intersects(rect2));
}

TEST(QuadTreeTest, RootAccessorReturnsTheConstructedRoot) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTree quadTree(boundary, 4);

    ASSERT_NE(quadTree.getRoot(), nullptr);
    EXPECT_TRUE(quadTree.getRoot()->isLeaf());
    EXPECT_TRUE(quadTree.getRoot()->getChildren().empty());
}

TEST(QuadTreeTest, InsertReportsPointsInsideAndOutsideTheBoundary) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTree quadTree(boundary, 4);
    PhysicsBody inside = createPoint(50, 50);
    PhysicsBody outside = createPoint(150, 50);

    EXPECT_TRUE(quadTree.insert(inside));
    EXPECT_FALSE(quadTree.insert(outside));
}

TEST(QuadTreeTest, QueryReturnsEmptyWhenRangeDoesNotIntersectBoundary) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTree quadTree(boundary, 4);
    PhysicsBody point = createPoint(50, 50);
    quadTree.insert(point);
    Rectangle range = createRectangle(200, 200, 10, 10);
    std::vector<PhysicsBody> found;

    const std::vector<PhysicsBody> result = quadTree.query(range, found);

    EXPECT_TRUE(found.empty());
    EXPECT_TRUE(result.empty());
}

TEST(QuadTreeTest, QueryAppendsToExistingResults) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTree quadTree(boundary, 4);
    PhysicsBody existing = createPoint(-10, -10);
    std::vector<PhysicsBody> found{existing};
    Rectangle range = createRectangle(0, 0, 100, 100);

    const std::vector<PhysicsBody> result = quadTree.query(range, found);

    ASSERT_EQ(found.size(), 1U);
    EXPECT_EQ(found.front().position.x(), S(-10));
    EXPECT_EQ(found.front().position.y(), S(-10));
    EXPECT_EQ(result.size(), found.size());
}

TEST(QuadTreeTest, ClearOnEmptyTreeAndReuseAfterClear) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTree quadTree(boundary, 4);
    quadTree.clear();

    PhysicsBody point = createPoint(25, 25);
    EXPECT_TRUE(quadTree.insert(point));
}

TEST(QuadTreeTest, ReBuildReplacesPopulatedRootWithEmptyRootAndNewBoundary) {
    Rectangle originalBoundary = createRectangle(0, 0, 100, 100);
    QuadTree quadTree(originalBoundary, 1);
    PhysicsBody first = createPoint(10, 10);
    PhysicsBody second = createPoint(90, 90);
    ASSERT_TRUE(quadTree.insert(first));
    ASSERT_TRUE(quadTree.insert(second));
    ASSERT_FALSE(quadTree.getRoot()->isLeaf());

    Rectangle newBoundary = createRectangle(200, 300, 40, 60);
    quadTree.ReBuild(newBoundary, 4);

    ASSERT_NE(quadTree.getRoot(), nullptr);
    EXPECT_TRUE(quadTree.getRoot()->isLeaf());
    EXPECT_TRUE(quadTree.getRoot()->getChildren().empty());
    EXPECT_EQ(quadTree.getRoot()->getBoundary().getPosition().x(), S(200));
    EXPECT_EQ(quadTree.getRoot()->getBoundary().getPosition().y(), S(300));
    EXPECT_EQ(quadTree.getRoot()->getBoundary().getWidth(), 40);
    EXPECT_EQ(quadTree.getRoot()->getBoundary().getHeight(), 60);

    Rectangle queryRange = createRectangle(200, 300, 40, 60);
    std::vector<PhysicsBody> found;
    quadTree.query(queryRange, found);
    EXPECT_TRUE(found.empty());
}

TEST(QuadTreeTest, ReBuildUsesNewCapacity) {
    Rectangle originalBoundary = createRectangle(0, 0, 100, 100);
    QuadTree quadTree(originalBoundary, 4);
    Rectangle newBoundary = createRectangle(200, 200, 100, 100);

    quadTree.ReBuild(newBoundary, 1);

    PhysicsBody first = createPoint(210, 210);
    PhysicsBody second = createPoint(290, 290);
    ASSERT_TRUE(quadTree.insert(first));
    EXPECT_TRUE(quadTree.getRoot()->isLeaf());
    ASSERT_TRUE(quadTree.insert(second));
    EXPECT_FALSE(quadTree.getRoot()->isLeaf());
    EXPECT_EQ(quadTree.getRoot()->getChildren().size(), 4U);
}

TEST(QuadTreeNodeTest, ConstructorExposesBoundaryAndStartsWithoutChildren) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTreeNode node(boundary, 4);
    PhysicsBody inside = createPoint(50, 50);
    PhysicsBody outside = createPoint(150, 50);

    EXPECT_TRUE(node.isLeaf());
    EXPECT_TRUE(node.getChildren().empty());
    EXPECT_TRUE(node.getBoundary().contains(inside));
    EXPECT_FALSE(node.getBoundary().contains(outside));
}

TEST(QuadTreeNodeTest, InsertRejectsPointsOutsideEachSide) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTreeNode node(boundary, 4);
    std::vector<PhysicsBody> points{
        createPoint(-1, 50), createPoint(101, 50),
        createPoint(50, -1), createPoint(50, 101)};

    for (auto& point : points) {
        EXPECT_FALSE(node.insert(point));
    }
}

TEST(QuadTreeNodeTest, InsertAcceptsPointOnBoundary) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTreeNode node(boundary, 4);
    PhysicsBody topLeft = createPoint(0, 0);
    PhysicsBody bottomRight = createPoint(100, 100);

    EXPECT_TRUE(node.insert(topLeft));
    EXPECT_TRUE(node.insert(bottomRight));
}

TEST(QuadTreeNodeTest, SubdivideEmptyNodeCreatesFourChildrenAndIsIdempotent) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTreeNode node(boundary, 4);

    node.subdivide();
    ASSERT_EQ(node.getChildren().size(), 4U);
    for (const auto& child : node.getChildren()) {
        EXPECT_NE(child, nullptr);
    }

    node.subdivide();
    EXPECT_EQ(node.getChildren().size(), 4U);
    EXPECT_FALSE(node.isLeaf());
}

TEST(QuadTreeNodeTest, InsertBelowCapacityDoesNotSubdivide) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTreeNode node(boundary, 4);

    for (int i = 0; i < 4; ++i) {
        PhysicsBody point = createPoint(i + 10, i + 10);
        EXPECT_TRUE(node.insert(point));
    }

    EXPECT_TRUE(node.isLeaf());
    EXPECT_TRUE(node.getChildren().empty());
}

TEST(QuadTreeNodeTest, InsertAtCapacityAndBeyondCapacitySubdivides) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTreeNode node(boundary, 1);
    PhysicsBody first = createPoint(10, 10);
    PhysicsBody second = createPoint(90, 90);

    EXPECT_TRUE(node.insert(first));
    EXPECT_TRUE(node.isLeaf());
    EXPECT_TRUE(node.insert(second));
    EXPECT_FALSE(node.isLeaf());
    EXPECT_EQ(node.getChildren().size(), 4U);
}

TEST(QuadTreeNodeTest, QueryForRangeOutsideBoundaryDoesNotChangeExistingResults) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTreeNode node(boundary, 4);
    std::vector<PhysicsBody> found{createPoint(-5, -5)};
    Rectangle range = createRectangle(200, 200, 10, 10);

    const std::vector<PhysicsBody> result = node.query(range, found);

    ASSERT_EQ(found.size(), 1U);
    EXPECT_EQ(found.front().position.x(), S(-5));
    EXPECT_EQ(result.size(), 1U);
}

TEST(QuadTreeNodeTest, QueryCanBeRepeatedAndAccumulatesResults) {
    Rectangle boundary = createRectangle(0, 0, 100, 100);
    QuadTreeNode node(boundary, 4);
    PhysicsBody point = createPoint(50, 50);
    node.insert(point);
    Rectangle range = createRectangle(0, 0, 100, 100);
    std::vector<PhysicsBody> found;

    node.query(range, found);
    node.query(range, found);

    EXPECT_EQ(found.size(), 2U);
}

TEST(RectangleTest, ContainsRejectsPointsBeyondEachSide) {
    Rectangle rectangle = createRectangle(0, 0, 100, 100);
    std::vector<PhysicsBody> points{
        createPoint(-1, 50), createPoint(101, 50),
        createPoint(50, -1), createPoint(50, 101)};

    for (auto& point : points) {
        EXPECT_FALSE(rectangle.contains(point));
    }
}

TEST(RectangleTest, ContainsIncludesAllFourCorners) {
    Rectangle rectangle = createRectangle(0, 0, 100, 100);
    std::vector<PhysicsBody> points{
        createPoint(0, 0), createPoint(100, 0),
        createPoint(0, 100), createPoint(100, 100)};

    for (auto& point : points) {
        EXPECT_TRUE(rectangle.contains(point));
    }
}

TEST(RectangleTest, IntersectsReturnsFalseWhenSeparatedOnEitherAxis) {
    Rectangle rectangle = createRectangle(0, 0, 100, 100);
    std::vector<Rectangle> disjoint{
        createRectangle(101, 0, 10, 10), createRectangle(-11, 0, 10, 10),
        createRectangle(0, 101, 10, 10), createRectangle(0, -11, 10, 10)};

    for (auto& other : disjoint) {
        EXPECT_FALSE(rectangle.intersects(other));
    }
}

TEST(RectangleTest, IntersectsIsSymmetricForDisjointAndOverlappingRectangles) {
    Rectangle first = createRectangle(0, 0, 100, 100);
    Rectangle overlapping = createRectangle(90, 90, 20, 20);
    Rectangle disjoint = createRectangle(200, 200, 10, 10);
    Rectangle disjointInReverse = createRectangle(200, 200, 10, 10);

    EXPECT_TRUE(first.intersects(overlapping));
    EXPECT_TRUE(overlapping.intersects(first));
    EXPECT_FALSE(first.intersects(disjoint));
    EXPECT_FALSE(disjointInReverse.intersects(first));
}

}}