#include "main/samples/04_trigger.h"

#include <array>

#include "gpr925/scalar.h"

#include "main/samples/sample_config.h"
#include "main/samples/sample_rng.h"

namespace gpr925 {
namespace {

constexpr std::uint32_t kSeed = 0x04B0115u;
constexpr Scalar kBallRadius = Ratio(3, 10);
constexpr Scalar kBallSpeed = Ratio(5, 2);
constexpr std::array<Vec2, 8> kDirections = {{
    {ToScalar(1), Scalar{}},
    {ToScalar(-1), Scalar{}},
    {Scalar{}, ToScalar(1)},
    {Scalar{}, ToScalar(-1)},
    {Ratio(707, 1000), Ratio(707, 1000)},
    {Ratio(-707, 1000), Ratio(707, 1000)},
    {Ratio(707, 1000), Ratio(-707, 1000)},
    {Ratio(-707, 1000), Ratio(-707, 1000)},
}};

ColorId NextColor(ColorId color) {
  return color == sample_color::kLastBright
             ? sample_color::kFirstBright
             : static_cast<ColorId>(color + 1);
}

}  // namespace

void TriggerSample::Begin(const SampleContext& context) {
  bounds_ = context.world_bounds;
  world_.SetBounds(bounds_);
  world_.SetGravity({});
  world_.Reserve(sample_config::kTriggerBallCount);
  colors_.reserve(sample_config::kTriggerBallCount);
  ball_bodies_.reserve(sample_config::kTriggerBallCount);
  paused_ = false;
  ResetScene();
}

void TriggerSample::FixedUpdate(const Input& input) {
  if (input.WasPressed(Button::kB)) {
    ResetScene();
    paused_ = false;
    return;
  }
  if (input.WasPressed(Button::kA)) {
    paused_ = !paused_;
  }
  if (paused_) {
    return;
  }

  world_.Step(kFixedDeltaTime);
  // Every moving body is a trigger. Advance both colors when a new
  // ball-to-ball overlap begins; the world never resolves these overlaps.
  for (std::size_t i = 0; i < ball_bodies_.size(); ++i) {
    PhysicsBody& ball = world_.body(ball_bodies_[i]);
    if (ball.contact_started_count > 0) {
      colors_[i] = NextColor(colors_[i]);
    }
  }
}

void TriggerSample::Draw(DrawList& draw_list) const {
  for (std::size_t i = 0; i < ball_bodies_.size(); ++i) {
    const PhysicsBody& ball = world_.body(ball_bodies_[i]);
    draw_list.circles.push_back(
        {{ball.position.x(), ball.position.y()}, ball.collider.radius,
         colors_[i]});
  }
}

void TriggerSample::End() {
  world_.Clear();
  colors_.clear();
  ball_bodies_.clear();
  bounds_ = {};
  paused_ = false;
}

void TriggerSample::ResetScene() {
  world_.Clear();
  colors_.clear();
  ball_bodies_.clear();
  SampleRng rng{kSeed};

  for (std::size_t i = 0; i < sample_config::kTriggerBallCount; ++i) {
    const Vec2 position{
        rng.Range(bounds_.min_bound.x() + ToScalar(1),
                  bounds_.max_bound.x() - ToScalar(1)),
        rng.Range(bounds_.min_bound.y() + ToScalar(1),
                  bounds_.max_bound.y() - ToScalar(1))};
    const Vec2 direction = kDirections[i % kDirections.size()];
    const Vec2 velocity =
        direction * Vec2{kBallSpeed, kBallSpeed};
    ball_bodies_.push_back(
        world_.AddTriggerBody(position, velocity, kBallRadius));
    colors_.push_back(rng.RandomColor());
  }
}

}  // namespace gpr925
