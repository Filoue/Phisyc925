#ifndef MAIN_SAMPLES_SAMPLE_TEMPLATE_H_
#define MAIN_SAMPLES_SAMPLE_TEMPLATE_H_

#include "main/samples/sample.h"

namespace gpr925 {

/// Starter class for a new platform-neutral sample.
///
/// Copy this file and @c sample_template.cc, rename the files and class, then
/// implement the sample's simulation and drawing.
class PlanetFourVec2 : public Sample {
 public:
  /// Initializes the sample from the host's world bounds.
  void Begin(const SampleContext& context) override;
  /// Advances the simulation by one fixed step.
  void FixedUpdate(const Input& input) override;
  /// Adds the sample's shapes to the draw list.
  void Draw(DrawList& draw_list) const override;
  /// Clears state before the sample is restarted or destroyed.
  void End() override;

 private:
  Rect bounds_{};
};

}  // namespace gpr925

#endif  // MAIN_SAMPLES_SAMPLE_TEMPLATE_H_
