#include "main/samples/06_QuadTree.h"

#include <cstdint>

#include "gpr925/scalar.h"

#include "main/samples/sample_config.h"
#include "main/samples/sample_rng.h"

namespace gpr925 {
namespace {

constexpr std::uint32_t kSeed = 0x06C0FFEEu;
constexpr std::size_t kBallColumns = 8;
constexpr std::size_t kBallRows =
    sample_config::kQuadTreeBallCount / kBallColumns;
constexpr std::size_t kQuadTreeCapacity = 4;
constexpr Scalar kBallRadius = Ratio(1, 10);
constexpr Scalar kMaxBallSpeed = ToScalar(2);
constexpr Scalar kTreeScale = ToScalar(10);
constexpr Scalar kTreeUnscale = Ratio(1, 10);
constexpr Vec2 kTreeScaleVector{kTreeScale, kTreeScale};
constexpr Vec2 kTreeUnscaleVector{kTreeUnscale, kTreeUnscale};

void DrawTreePartitions(QuadTreeNode& node, DrawList& draw_list) {
  Rectangle& boundary = node.getBoundary();
  if (!node.isLeaf() &&
      draw_list.lines.size() + 2 <= sample_config::kMaxDrawLines) {
    const Vec2 top_left = boundary.getPosition() * kTreeUnscaleVector;
    const Scalar width = ToScalar(boundary.getWidth()) * kTreeUnscale;
    const Scalar height = ToScalar(boundary.getHeight()) * kTreeUnscale;
    const Scalar middle_x =
        top_left.x() + ToScalar(boundary.getWidth() / 2) * kTreeUnscale;
    const Scalar middle_y =
        top_left.y() + ToScalar(boundary.getHeight() / 2) * kTreeUnscale;
    const Vec2 bottom_right{top_left.x() + width, top_left.y() + height};

    draw_list.lines.push_back(
        {{middle_x, top_left.y()}, {middle_x, bottom_right.y()},
         sample_color::kGrey});
    draw_list.lines.push_back(
        {{top_left.x(), middle_y}, {bottom_right.x(), middle_y},
         sample_color::kGrey});
  }

  for (const auto& child : node.getChildren()) {
    DrawTreePartitions(*child, draw_list);
  }
}

}  // namespace

void QuadTreeSample::Begin(const SampleContext& context) {
  world_.Clear();
  world_.Reserve(sample_config::kQuadTreeBallCount);
  bounds_ = context.world_bounds;
  world_.SetBounds(bounds_);

  SampleRng rng{kSeed};
  const Vec2 size = bounds_.Size();
  for (std::size_t row = 0; row < kBallRows; ++row) {
    for (std::size_t column = 0; column < kBallColumns; ++column) {
      const Vec2 position{
          bounds_.min_bound.x() +
              size.x() * Ratio(static_cast<int>(column + 1),
                               static_cast<int>(kBallColumns + 1)),
          bounds_.min_bound.y() +
              size.y() * Ratio(static_cast<int>(row + 1),
                               static_cast<int>(kBallRows + 1))};
      Vec2 velocity{ToScalar(rng.Range(-2, 2)),
                    ToScalar(rng.Range(-2, 2))};
      if (velocity == Vec2{}) {
        velocity.x() = ToScalar(1);
      }
      world_.AddBody(position, velocity, kBallRadius, ToScalar(1));
    }
  }

  const Vec2 scaled_min = bounds_.min_bound * kTreeScaleVector;
  const Vec2 scaled_size = size * kTreeScaleVector;
  Rectangle tree_bounds(scaled_min, ToInt(scaled_size.x()),
                        ToInt(scaled_size.y()));
  quad_tree_ = std::make_unique<QuadTree>(tree_bounds, kQuadTreeCapacity);
  RebuildQuadTree();
}

void QuadTreeSample::FixedUpdate(const Input&) {
  world_.Step(kFixedDeltaTime);
  RebuildQuadTree();
}

void QuadTreeSample::Draw(DrawList& draw_list) const {
  if (quad_tree_ != nullptr) {
    DrawTreePartitions(*quad_tree_->getRoot(), draw_list);
  }

  for (std::size_t i = 0; i < world_.body_count(); ++i) {
    const PhysicsBody& ball = world_.body(i);
    const ColorId color = ball.touching_count > 0 ? sample_color::kGreen
                                                  : sample_color::kRed;
    draw_list.circles.push_back(
        {ball.position, ball.collider.radius, color});
  }
}

void QuadTreeSample::End() {
  world_.Clear();
  quad_tree_.reset();
}

void QuadTreeSample::RebuildQuadTree() {
  if (quad_tree_ == nullptr) {
    return;
  }

  quad_tree_->clear();
  for (std::size_t i = 0; i < world_.body_count(); ++i) {
    PhysicsBody point = world_.body(i);
    point.position *= kTreeScaleVector;
    quad_tree_->insert(point);
  }
}

}  // namespace gpr925
