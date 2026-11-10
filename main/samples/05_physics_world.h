#ifndef MAIN_SAMPLES_05_PHYSICS_WORLD_H_
#define MAIN_SAMPLES_05_PHYSICS_WORLD_H_

#include <cstddef>

#include "gpr925/physics_world.h"

#include "main/samples/sample.h"

namespace gpr925 {

/// Demonstrates gravity, bouncing bodies, impulses and contact events.
class PhysicsWorldSample : public Sample, public PhysicsContactListener {
 public:
  /// Creates solid bodies and a trigger volume.
  void Begin(const SampleContext& context) override;
  /// Advances the world and applies an impulse when A is pressed.
  void FixedUpdate(const Input& input) override;
  /// Draws colliders and the trigger volume.
  void Draw(DrawList& draw_list) const override;
  /// Removes the bodies and resets the world.
  void End() override;
  /// Handles a new overlap with a trigger.
  void OnTriggerEnter(std::size_t body_index,
                      std::size_t other_body_index) override;
  /// Handles an overlap ending with a trigger.
  void OnTriggerExit(std::size_t body_index,
                     std::size_t other_body_index) override;
  /// Handles a new solid collision.
  void OnColliderEnter(std::size_t body_index,
                       std::size_t other_body_index) override;
  /// Handles a solid collision ending.
  void OnColliderExit(std::size_t body_index,
                      std::size_t other_body_index) override;

 private:
  PhysicsWorld world_;
  std::size_t first_body_ = 0;
  std::size_t second_body_ = 0;
  std::size_t trigger_body_ = 0;
  bool trigger_entered_ = false;
  bool trigger_exited_ = false;
  bool collider_entered_ = false;
  bool collider_exited_ = false;
};

}  // namespace gpr925

#endif  // MAIN_SAMPLES_05_PHYSICS_WORLD_H_
