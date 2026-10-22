#include "main/samples/03_planetFourVec2.h"

namespace gpr925 {

void PlanetFourVec2::Begin(const SampleContext& context) {
  bounds_ = context.world_bounds;
}

void PlanetFourVec2::FixedUpdate(const Input&) {}

void PlanetFourVec2::Draw(DrawList&) const {}

void PlanetFourVec2::End() { bounds_ = {}; }

}  // namespace gpr925
