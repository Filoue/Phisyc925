#ifndef MAIN_SAMPLES_06_QUAD_TREE_H_
#define MAIN_SAMPLES_06_QUAD_TREE_H_

#include <memory>

#include "gpr925/physics_world.h"
#include "gpr925/quad_tree.h"

#include "main/samples/sample.h"

namespace gpr925 {

/// Shows bouncing balls and the changing QuadTree that contains them.
class QuadTreeSample : public Sample {
 public:
  /// Places the balls and creates the initial QuadTree.
  void Begin(const SampleContext& context) override;
  /// Advances the balls and rebuilds the QuadTree from their positions.
  void FixedUpdate(const Input& input) override;
  /// Draws the balls and the QuadTree partitions.
  void Draw(DrawList& draw_list) const override;
  /// Clears the physics scene and QuadTree.
  void End() override;

 private:
  void RebuildQuadTree();

  PhysicsWorld world_;
  Rect bounds_{};
  std::unique_ptr<QuadTree> quad_tree_;
};

}  // namespace gpr925

#endif  // MAIN_SAMPLES_06_QUAD_TREE_H_
