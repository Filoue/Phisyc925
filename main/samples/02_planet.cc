#include "main/samples/02_planet.h"

#include "gpr925/scalar.h"

#include "main/samples/sample_rng.h"

namespace gpr925 {
namespace {

constexpr std::uint8_t kAngleSteps = 64;
constexpr std::uint8_t kAngularSpeed = 1;
constexpr std::uint32_t kSeed = 1234;
constexpr Scalar kMinOrbitFraction = Ratio(8, 100);
constexpr Scalar kMaxOrbitFraction = Ratio(44, 100);
constexpr Scalar kStepCos = Scalar{0.9951847f};
constexpr Scalar kStepSin = Scalar{0.0980171f};

constexpr Vec2 Scale(Vec2 vector, Scalar scale) {
  return vector * Vec2{scale, scale};
}

constexpr Vec2 RotateOneStep(Vec2 direction) {
  return {direction.x * kStepCos - direction.y * kStepSin,
          direction.x * kStepSin + direction.y * kStepCos};
}

Vec2 Normalize(Vec2 direction) {
  const Scalar length = Sqrt(direction.SqrMagnitude());
  return {direction.x / length, direction.y / length};
}

constexpr Vec2 DirectionAt(std::uint8_t phase) {
  Vec2 direction{ToScalar(1), Scalar{}};
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
    planet.direction = RotateOneStep(planet.direction);
    if (planet.phase == 0) {
      planet.direction = Normalize(planet.direction);
    }
    planet.position = center_ +
                      Scale(planet.direction, planet.orbit_radius);
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
    planet.direction = DirectionAt(planet.phase);
    planet.radius = Ratio(1, 10);
    planet.color = rng.RandomColor();
    planet.position =
        center_ + Scale(planet.direction, planet.orbit_radius);
  }
}

}  // namespace gpr925
