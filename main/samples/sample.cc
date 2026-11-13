#include "main/samples/sample.h"

#include "main/samples/01_hello.h"
#include "main/samples/02_planet.h"
#include "main/samples/03_planetFourVec2.h"
#include "main/samples/04_trigger.h"
#include "main/samples/05_Collision.h"
#include "main/samples/06_QuadTree.h"

namespace gpr925 {

SampleRegistry::SampleRegistry() {
  // Register your samples here: bump the reserve and add a push_back
  samples_.reserve(6);
  samples_.push_back({"01 Hello", std::make_unique<HelloSample>()});
  samples_.push_back({"02 Planet", std::make_unique<PlanetSample>()});
  samples_.push_back({"03 Planet FourVec2", std::make_unique<PlanetFourVec2>()});
  samples_.push_back({"04 Trigger", std::make_unique<TriggerSample>()});
  samples_.push_back({"05 Collision", std::make_unique<CollisionSample>()});
  samples_.push_back({"06 QuadTree", std::make_unique<QuadTreeSample>()});
}

SampleRegistry::~SampleRegistry() { Select(kNone, context_); }

void SampleRegistry::Select(std::size_t index, const SampleContext& context) {
  if (current_index_ != kNone) {
    samples_[current_index_].sample->End();
  }
  current_index_ = index < samples_.size() ? index : kNone;
  context_ = context;
  if (current_index_ != kNone) {
    samples_[current_index_].sample->Begin(context_);
  }
}

void SampleRegistry::FixedUpdate(const Input& input) {
  if (current_index_ != kNone) {
    samples_[current_index_].sample->FixedUpdate(input);
  }
}

void SampleRegistry::Draw(DrawList& draw_list) const {
  draw_list.Clear();
  draw_list.camera = context_.world_bounds;
  if (current_index_ != kNone) {
    samples_[current_index_].sample->Draw(draw_list);
  }
}

}  // namespace gpr925
