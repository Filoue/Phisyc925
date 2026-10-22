#include "main/samples/02_planet.h"

#include "gpr925/scalar.h"
#include "vector2.h"

#include "main/samples/sample_rng.h"

namespace maths {

template <>
struct NumberTraits<gpr925::Fixed> {
  static constexpr bool kIsExact = true;
  static constexpr gpr925::Fixed kEpsilon{};

  [[nodiscard]] static constexpr gpr925::Fixed Sqrt(
      gpr925::Fixed value) noexcept {
    return gpr925::Sqrt(value);
  }
  [[nodiscard]] static constexpr gpr925::Fixed Abs(
      gpr925::Fixed value) noexcept {
    return gpr925::Abs(value);
  }
};

}  // namespace maths

namespace gpr925 {
namespace {

using PlanetVector = maths::Vector2<Scalar>;

constexpr std::uint8_t kAngleSteps = 64;
constexpr std::uint8_t kAngularSpeed = 1;
constexpr std::uint32_t kSeed = 1234;
constexpr Scalar kMinOrbitFraction = Ratio(8, 100);
constexpr Scalar kMaxOrbitFraction = Ratio(44, 100);
constexpr Scalar kStepCos = Scalar{0.9951847f};
constexpr Scalar kStepSin = Scalar{0.0980171f};

constexpr PlanetVector ToPlanetVector(Vec2 vector) {
  return {vector.x, vector.y};
}

constexpr Vec2 ToVec2(PlanetVector vector) {
  return {vector.x(), vector.y()};
}

constexpr PlanetVector Scale(PlanetVector vector, Scalar scale) {
  return vector * scale;
}

constexpr PlanetVector RotateOneStep(PlanetVector direction) {
  return {direction.x() * kStepCos - direction.y() * kStepSin,
          direction.x() * kStepSin + direction.y() * kStepCos};
}

PlanetVector Normalize(PlanetVector direction) {
  const Scalar length = Sqrt(direction.SqrMagnitude());
  return {direction.x() / length, direction.y() / length};
}

constexpr PlanetVector DirectionAt(std::uint8_t phase) {
  PlanetVector direction{ToScalar(1), Scalar{}};
  for (std::uint8_t step = 0; step < phase; ++step) {
    direction = RotateOneStep(direction);
  }
  return Normalize(direction);
}

}

void PlanetSample::Begin(const SampleContext& context) {
  bounds_ = context.world_bounds;
  center_ = bounds_.Center();
  paused_ = false;
  ResetPlanets();
}

void PlanetSample::FixedUpdate(const Input& input) {
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

  for (Planet& planet : planets_) {
    planet.phase = static_cast<std::uint8_t>(
        (planet.phase + kAngularSpeed) % kAngleSteps);
    PlanetVector direction = RotateOneStep(ToPlanetVector(planet.direction));
    if (planet.phase == 0) {
      direction = Normalize(direction);
    }
    planet.direction = ToVec2(direction);
    planet.position = center_ +
                      ToVec2(Scale(direction, planet.orbit_radius));
  }
}

void PlanetSample::Draw(DrawList& draw_list) const {
  for (const Planet& planet : planets_) {
    draw_list.circles.push_back(
        {planet.position, planet.radius, planet.color});
  }
}

void PlanetSample::End() {
  planets_ = {};
  center_ = {};
  paused_ = false;
}

void PlanetSample::ResetPlanets() {
  SampleRng rng{kSeed};
  const Scalar min_radius = bounds_.Size().y * kMinOrbitFraction;
  const Scalar max_radius = bounds_.Size().y * kMaxOrbitFraction;
  for (Planet& planet : planets_) {
    planet.orbit_radius = rng.Range(min_radius, max_radius);
    planet.phase = static_cast<std::uint8_t>(rng.Range(0, kAngleSteps - 1));
    planet.direction = ToVec2(DirectionAt(planet.phase));
    planet.radius = Ratio(1, 10);
    planet.color = rng.RandomColor();
    planet.position = center_ + ToVec2(Scale(ToPlanetVector(planet.direction), planet.orbit_radius));
  }
}

}  // namespace gpr925
