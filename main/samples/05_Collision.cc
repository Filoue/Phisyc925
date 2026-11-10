#include "main/samples/05_Collision.h"

#include <array>

#include "gpr925/scalar.h"

#include "main/samples/sample_config.h"

namespace gpr925 {
namespace {

constexpr std::array<Scalar, 4> kBallRadii = {
    Ratio(1, 5), Ratio(1, 4), Ratio(3, 10), Ratio(7, 20)};
constexpr std::size_t kBallColumns = 16;
constexpr std::size_t kBallRows =
    sample_config::kCollisionBallCount / kBallColumns;
constexpr Scalar kGravity = Ratio(981, 100);
constexpr Scalar kBallMass = ToScalar(1);

}  // namespace

void CollisionSample::Begin(const SampleContext& context) {
  world_.Clear();
  world_.Reserve(sample_config::kCollisionBallCount);
  world_.SetBounds(context.world_bounds);
  world_.SetGravity({Scalar{}, kGravity});
  colors_.clear();
  colors_.reserve(sample_config::kCollisionBallCount);

  const Rect& bounds = context.world_bounds;
  const Scalar width = bounds.max_bound.x - bounds.min_bound.x;
  const Scalar height = bounds.max_bound.y - bounds.min_bound.y;
  const Scalar spacing_height = height - Ratio(7, 10);
  for (std::size_t row = 0; row < kBallRows; ++row) {
    for (std::size_t column = 0; column < kBallColumns; ++column) {
      const Scalar x = bounds.min_bound.x +
                       width * Ratio(static_cast<int>(column + 1), 17);
      const Scalar y =
          bounds.min_bound.y + Ratio(7, 20) +
          spacing_height * Ratio(static_cast<int>(row), 7);
      const Scalar radius = kBallRadii[(row + column) % kBallRadii.size()];
      world_.AddBody({x, y}, {}, radius, kBallMass);
      colors_.push_back(sample_color::kRed);
    }
  }
}

void CollisionSample::FixedUpdate(const Input&) {
  world_.Step(kFixedDeltaTime);
  for (std::size_t i = 0; i < colors_.size(); ++i) {
    colors_[i] = world_.body(i).touching_count > 0 ? sample_color::kGreen
                                                   : sample_color::kRed;
  }
}

void CollisionSample::Draw(DrawList& draw_list) const {
  for (std::size_t i = 0; i < colors_.size(); ++i) {
    const PhysicsBody& ball = world_.body(i);
    draw_list.circles.push_back(
        {{ball.position.x(), ball.position.y()}, ball.collider.radius,
         colors_[i]});
  }
}

void CollisionSample::End() {
  world_.Clear();
  colors_.clear();
}

}  // namespace gpr925