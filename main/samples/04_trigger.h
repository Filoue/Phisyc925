#ifndef MAIN_SAMPLES_04_TRIGGER_H_
#define MAIN_SAMPLES_04_TRIGGER_H_

#include <cstddef>
#include <vector>

#include "gpr925/physics_world.h"

#include "main/samples/sample.h"

namespace gpr925 {

/// Demonstrates balls changing colors when their trigger areas overlap.
class TriggerSample : public Sample {
 public:
  /// Creates larger balls moving at the same speed.
  void Begin(const SampleContext& context) override;
  /// Advances the trigger world or handles pause and reset input.
  void FixedUpdate(const Input& input) override;
  /// Draws the balls in their current colors.
  void Draw(DrawList& draw_list) const override;
  /// Clears the scene.
  void End() override;

 private:
  void ResetScene();

  Rect bounds_{};
  PhysicsWorld world_;
  std::vector<ColorId> colors_;
  std::vector<std::size_t> ball_bodies_;
  bool paused_ = false;
};

}  // namespace gpr925

#endif  // MAIN_SAMPLES_04_TRIGGER_H_
