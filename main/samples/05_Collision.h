#ifndef MAIN_SAMPLES_05_COLLISION_H_
#define MAIN_SAMPLES_05_COLLISION_H_

#include <vector>

#include "gpr925/physics_world.h"

#include "main/samples/sample.h"

namespace gpr925 {

/// Shows differently sized balls that fall and turn green while touching.
class CollisionSample : public Sample {
 public:
  /// Creates a grid of differently sized balls above the ground.
  void Begin(const SampleContext& context) override;
  /// Advances the falling balls and colors contacts green.
  void FixedUpdate(const Input& input) override;
  /// Draws each ball in its current color.
  void Draw(DrawList& draw_list) const override;
  /// Clears the physics scene.
  void End() override;

 private:
  PhysicsWorld world_;
  std::vector<ColorId> colors_;
};

}  // namespace gpr925

#endif  // MAIN_SAMPLES_05_COLLISION_H_