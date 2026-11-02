#ifndef GPR925_ENGINE_LIB_NUMBER_TRAITS_H_
#define GPR925_ENGINE_LIB_NUMBER_TRAITS_H_

#include "gpr925/fixed.h"

#include "number_concepts.h"

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

#endif  // GPR925_ENGINE_LIB_NUMBER_TRAITS_H_
