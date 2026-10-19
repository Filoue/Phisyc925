#include "main/samples/02_planet.h"

#include "gpr925/scalar.h"

#include "main/samples/sample_rng.h"

namespace gpr925 {
namespace {

constexpr std::uint8_t kAngleSteps = 64;
constexpr std::uint8_t kAngularSpeed = 1;
constexpr std::uint32_t kSeed = 0xB1AC0FFEu;
constexpr Scalar kMinOrbitFraction = Ratio(8, 100);
constexpr Scalar kMaxOrbitFraction = Ratio(44, 100);
constexpr std::array<Vec2, 17> kQuarterCircle = {{
    {ToScalar(1), Scalar{}},
    {Ratio(9952, 10000), Ratio(980, 10000)},
    {Ratio(9808, 10000), Ratio(1951, 10000)},
    {Ratio(9569, 10000), Ratio(2903, 10000)},
    {Ratio(9239, 10000), Ratio(3827, 10000)},
    {Ratio(8819, 10000), Ratio(4714, 10000)},
    {Ratio(8315, 10000), Ratio(5556, 10000)},
    {Ratio(7730, 10000), Ratio(6344, 10000)},
    {Ratio(7071, 10000), Ratio(7071, 10000)},
    {Ratio(6344, 10000), Ratio(7730, 10000)},
    {Ratio(5556, 10000), Ratio(8315, 10000)},
    {Ratio(4714, 10000), Ratio(8819, 10000)},
    {Ratio(3827, 10000), Ratio(9239, 10000)},
    {Ratio(2903, 10000), Ratio(9569, 10000)},
    {Ratio(1951, 10000), Ratio(9808, 10000)},
    {Ratio(980, 10000), Ratio(9952, 10000)},
    {Scalar{}, ToScalar(1)},
}};

constexpr Vec2 Scale(Vec2 vector, Scalar scale) {
  return vector * Vec2{scale, scale};
}

constexpr Vec2 DirectionAt(std::uint8_t phase) {
  phase = static_cast<std::uint8_t>(phase % kAngleSteps);
  if (phase <= 16) {
    return kQuarterCircle[phase];
  }
  if (phase <= 32) {
    const Vec2 direction = kQuarterCircle[phase - 16];
    return {-direction.y, direction.x};
  }
  if (phase <= 48) {
    const Vec2 direction = kQuarterCircle[phase - 32];
    return {-direction.x, -direction.y};
  }
  const Vec2 direction = kQuarterCircle[phase - 48];
  return {direction.y, -direction.x};
}

}  // namespace

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
    planet.position = center_ +
                      Scale(DirectionAt(planet.phase), planet.orbit_radius);
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
    planet.radius = Ratio(1, 10);
    planet.color = rng.RandomColor();
    planet.position =
        center_ + Scale(DirectionAt(planet.phase), planet.orbit_radius);
  }
}

}  // namespace gpr925
