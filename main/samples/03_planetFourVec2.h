#ifndef MAIN_SAMPLES_03_PLANET_FOUR_VEC2_H_
#define MAIN_SAMPLES_03_PLANET_FOUR_VEC2_H_

#include <array>
#include <cstddef>

#include "four_vector2.h"

#include "gpr925/vec2.h"

#include "main/samples/sample.h"

namespace maths {

template <>
struct NumberTraits<gpr925::Fixed> {
  static constexpr bool kIsExact = false;
  static constexpr gpr925::Fixed kEpsilon = gpr925::Fixed::FromRaw(1);

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

class PlanetFourVec2 : public Sample {
 public:

  void Begin(const SampleContext& context) override;

  void FixedUpdate(const Input& input) override;

  void Draw(DrawList& draw_list) const override;

  void End() override;

 private:
  static constexpr std::size_t kPlanetCount = 128;
  static constexpr std::size_t kBatchSize = 4;
  static constexpr std::size_t kBatchCount = kPlanetCount / kBatchSize;

  void ResetPlanets();

  Rect bounds_{};
  maths::Vector2<Scalar> center_{};
  std::array<maths::FourVector2<Scalar>, kBatchCount> positions_{};
  std::array<maths::FourVector2<Scalar>, kBatchCount> velocities_{};
  std::array<ColorId, kPlanetCount> colors_{};
  bool paused_ = false;
};

}  // namespace gpr925

#endif  // MAIN_SAMPLES_03_PLANET_FOUR_VEC2_H_
