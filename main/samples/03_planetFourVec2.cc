#include "main/samples/03_planetFourVec2.h"

#include "gpr925/scalar.h"

#include "main/samples/sample_rng.h"

namespace gpr925 {
namespace {

constexpr std::uint32_t kSeed = 1234;
constexpr Scalar kGravityParameter = Ratio(20, 1);
constexpr Scalar kMinOrbitFraction = Ratio(8, 100);
constexpr Scalar kMaxOrbitFraction = Ratio(30, 100);
constexpr Scalar kSofteningSquared = Ratio(1, 100);
constexpr Scalar kPlanetRadius = Ratio(1, 10);

constexpr Scalar Min(Scalar a, Scalar b) { return a < b ? a : b; }

using FourVec2 = maths::FourVector2<Scalar>;
using EngineVec2 = maths::Vector2<Scalar>;

FourVec2 GravityAcceleration(const FourVec2& positions) {
  const std::array<Scalar, 4> x = positions.Xs();
  const std::array<Scalar, 4> y = positions.Ys();
  const std::array<Scalar, 4> distance_squared =
      FourVec2::Dot(positions, positions);

  std::array<Scalar, 4> acceleration_x{};
  std::array<Scalar, 4> acceleration_y{};

  for (std::size_t lane = 0; lane < 4; ++lane) {
    const EngineVec2 displacement{x[lane], y[lane]};
    const Scalar softened_distance_squared =
        distance_squared[lane] + kSofteningSquared;
    const Scalar distance = Sqrt(softened_distance_squared);
    const Scalar scale =
        -kGravityParameter / (softened_distance_squared * distance);

    const EngineVec2 acceleration = displacement * scale;
    acceleration_x[lane] = acceleration.x();
    acceleration_y[lane] = acceleration.y();
  }
  return {acceleration_x, acceleration_y};
}

}  // namespace

void PlanetFourVec2::Begin(const SampleContext& context) {
  bounds_ = context.world_bounds;
  const Vec2 world_center = bounds_.Center();
  center_ = EngineVec2{world_center.x, world_center.y};
  paused_ = false;
  ResetPlanets();
}

void PlanetFourVec2::FixedUpdate(const Input& input) {
  if (input.WasPressed(Button::kB)) {
    ResetPlanets();
    paused_ = false;
    return;
  }
  if (input.WasPressed(Button::kA)) {
    paused_ = !paused_;
  }
  if (paused_) {
    return;
  }

  const FourVec2 delta_time{kFixedDeltaTime, kFixedDeltaTime};
  const FourVec2 half_delta_time{kFixedDeltaTime / ToScalar(2),
                                 kFixedDeltaTime / ToScalar(2)};
  for (std::size_t batch = 0; batch < kBatchCount; ++batch) {
    velocities_[batch] +=
        GravityAcceleration(positions_[batch]) * half_delta_time;
    positions_[batch] += velocities_[batch] * delta_time;
    velocities_[batch] +=
        GravityAcceleration(positions_[batch]) * half_delta_time;
  }
}

void PlanetFourVec2::Draw(DrawList& draw_list) const {
  for (std::size_t batch = 0; batch < kBatchCount; ++batch) {
    const std::array<Scalar, kBatchSize> x = positions_[batch].Xs();
    const std::array<Scalar, kBatchSize> y = positions_[batch].Ys();
    for (std::size_t lane = 0; lane < kBatchSize; ++lane) {
      const std::size_t planet = batch * kBatchSize + lane;
      const EngineVec2 world_position =
          center_ + EngineVec2{x[lane], y[lane]};
      draw_list.circles.push_back(
          {Vec2{world_position.x(), world_position.y()}, kPlanetRadius,
           colors_[planet]});
    }
  }
}

void PlanetFourVec2::End() {
  positions_ = {};
  velocities_ = {};
  colors_ = {};
  center_ = {};
  bounds_ = {};
  paused_ = false;
}

void PlanetFourVec2::ResetPlanets() {
  SampleRng rng{kSeed};
  const Scalar max_radius =
      Min(bounds_.Size().x, bounds_.Size().y) * kMaxOrbitFraction;
  const Scalar min_radius =
      max_radius * (kMinOrbitFraction / kMaxOrbitFraction);
  const Scalar max_radius_squared = max_radius * max_radius;
  const Scalar min_radius_squared = min_radius * min_radius;

  std::size_t planet = 0;
  for (std::size_t batch = 0; batch < kBatchCount; ++batch) {
    std::array<Scalar, kBatchSize> x{};
    std::array<Scalar, kBatchSize> y{};
    std::array<Scalar, kBatchSize> velocity_x{};
    std::array<Scalar, kBatchSize> velocity_y{};
    for (std::size_t lane = 0; lane < kBatchSize; ++lane, ++planet) {
      Scalar distance_squared{};
      do {
        x[lane] = rng.Range(-max_radius, max_radius);
        y[lane] = rng.Range(-max_radius, max_radius);
        distance_squared = x[lane] * x[lane] + y[lane] * y[lane];
      } while (distance_squared < min_radius_squared ||
               distance_squared > max_radius_squared);

      const Scalar softened_distance_squared =
          distance_squared + kSofteningSquared;
      const Scalar distance = Sqrt(softened_distance_squared);
      const Scalar orbital_speed =
          Sqrt(kGravityParameter * distance_squared /
               (softened_distance_squared * distance));
      const EngineVec2 displacement{x[lane], y[lane]};
      const EngineVec2 tangent{-displacement.y(), displacement.x()};
      const EngineVec2 velocity = tangent * (orbital_speed / distance);
      velocity_x[lane] = velocity.x();
      velocity_y[lane] = velocity.y();
      colors_[planet] = rng.RandomColor();
    }
    positions_[batch] = maths::FourVector2<Scalar>{x, y};
    velocities_[batch] =
        maths::FourVector2<Scalar>{velocity_x, velocity_y};
  }
}

}  // namespace gpr925
