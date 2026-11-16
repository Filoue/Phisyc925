#ifndef MAIN_SAMPLES_07_PLATFORMER_H_
#define MAIN_SAMPLES_07_PLATFORMER_H_

#include "main/samples/sample.h"

namespace gpr925 {

/// Three-screen platform game with a fixed camera and one screen per level.
class PlatformerSample : public Sample {
 public:
  /// Sets up the first level and places the player on the floor.
  void Begin(const SampleContext& context) override;
  /// Moves and jumps the player, handles platforms and advances levels.
  void FixedUpdate(const Input& input) override;
  /// Draws the current level, exit, player and level indicators.
  void Draw(DrawList& draw_list) const override;
  /// Resets the sample state.
  void End() override;

 private:
  Rect bounds_{};
  Vec2 player_position_{};
  Vec2 player_velocity_{};
  std::size_t level_ = 0;
  bool grounded_ = false;
  bool completed_ = false;
};

}  // namespace gpr925

#endif  // MAIN_SAMPLES_07_PLATFORMER_H_
