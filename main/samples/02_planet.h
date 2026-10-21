#ifndef MAIN_SAMPLES_02_PLANET_H_
#define MAIN_SAMPLES_02_PLANET_H_

#include <array>
#include <cstddef>
#include <cstdint>

#include "gpr925/vec2.h"

#include "main/samples/sample.h"

namespace gpr925 {

class PlanetSample : public Sample {
 public:
  /// Creates the planets around a black hole in the middle of the world.
  void Begin(const SampleContext& context) override;
  /// Advances the planets around the sun by one fixed step.
  void FixedUpdate(const Input& input) override;
  /// Draws the black hole and planets without fixed orbit paths.
  void Draw(DrawList& draw_list) const override;
  /// Clears the planets and pause state.
  void End() override;

 private:
  static constexpr std::size_t kPlanetCount = 128;

  struct Planet {
    Vec2 position{};
    Scalar orbit_radius{};
    Scalar radius{};
    ColorId color = sample_color::kWhite;
    std::uint8_t phase = 0;
  };

  void ResetPlanets();

  Rect bounds_{};
  Vec2 center_{};
  std::array<Planet, kPlanetCount> planets_{};
  bool paused_ = false;
};

}  // namespace gpr925

#endif  // MAIN_SAMPLES_02_PLANET_H_
