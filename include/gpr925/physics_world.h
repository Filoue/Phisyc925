#ifndef GPR925_PHYSICS_WORLD_H_
#define GPR925_PHYSICS_WORLD_H_

#include <cstddef>
#include <vector>

#include "gpr925/engine_lib_number_traits.h"
#include "gpr925/vec2.h"

#include "vector2.h"

namespace gpr925 {

/// Vector type used by EngineLib's math and physics algorithms.
using PhysicsVector = maths::Vector2<Scalar>;

/// A circular rigid body simulated by @ref PhysicsWorld.
struct PhysicsBody {
  PhysicsVector position{};       ///< Center of the body, in meters.
  PhysicsVector velocity{};       ///< Velocity, in meters per second.
  Scalar radius{};                 ///< Collision radius, in meters.
  Scalar mass = ToScalar(1);      ///< Mass; ignored for static bodies.
  Scalar restitution = ToScalar(1);  ///< Bounciness from zero to one.
  bool is_static = false;         ///< Static bodies do not move.
  bool is_trigger = false;        ///< Overlaps without collision response.
  bool contact_started = false;   ///< True on the first step of a new contact.

 private:
  PhysicsVector force_{};
  bool touching_ = false;
  bool was_touching_ = false;

  friend class PhysicsWorld;
};

/// Reusable 2D physics simulation for samples and other engine clients.
///
/// Supports gravity, accumulated forces, impulses, static and dynamic circular
/// bodies, circle-to-circle collision response, non-blocking circular triggers,
/// and optional rectangular world bounds. EngineLib's @ref maths::Vector2 is
/// used for all physics vectors.
///
/// A sample owns its world, configures it in @ref Sample::Begin, adds bodies,
/// applies forces or impulses in @ref Sample::FixedUpdate, steps it once per
/// fixed update, then reads body state to draw. Bounds are disabled until
/// @ref SetBounds is called.
class PhysicsWorld {
 public:
  /// Enables rectangular world boundaries.
  void SetBounds(Rect bounds) {
    bounds_ = bounds;
    has_bounds_ = true;
  }
  /// Disables rectangular world boundaries.
  void ClearBounds() { has_bounds_ = false; }
  /// Sets constant acceleration, in meters per second squared.
  void SetGravity(PhysicsVector gravity) { gravity_ = gravity; }
  /// Reserves body storage; call during setup to avoid simulation-time growth.
  void Reserve(std::size_t body_count) { bodies_.reserve(body_count); }
  /// Removes all bodies while retaining their allocated storage.
  void Clear() { bodies_.clear(); }

  /// Adds a movable circular body.
  ///
  /// @param position Center in meters.
  /// @param velocity In meters per second.
  /// @param radius Positive collision radius in meters.
  /// @param mass Positive mass; the world uses its reciprocal.
  /// @param restitution Bounciness from zero (no bounce) to one (elastic).
  /// @return Index used to retrieve the body from @ref body.
  ///
  /// Radius and mass must be positive. Restitution must be in [0, 1].
  std::size_t AddBody(PhysicsVector position, PhysicsVector velocity,
                      Scalar radius, Scalar mass,
                      Scalar restitution = ToScalar(1));

  /// Adds an immovable circular body, such as a wall or obstacle.
  ///
  /// @param position Center in meters.
  /// @param radius Positive collision radius in meters.
  /// @param restitution Bounciness from zero (no bounce) to one (elastic).
  /// @return Index used to retrieve the body from @ref body.
  std::size_t AddStaticBody(PhysicsVector position, Scalar radius,
                            Scalar restitution = ToScalar(1));

  /// Adds a movable circular trigger that reports overlaps without resolving
  /// them or changing either body's velocity.
  ///
  /// @param position Center in meters.
  /// @param velocity In meters per second.
  /// @param radius Positive trigger radius in meters.
  /// @return Index used to retrieve the body from @ref body.
  std::size_t AddTriggerBody(PhysicsVector position, PhysicsVector velocity,
                             Scalar radius);

  /// Adds an immovable circular trigger volume.
  ///
  /// @param position Center in meters.
  /// @param radius Positive trigger radius in meters.
  /// @return Index used to retrieve the body from @ref body.
  std::size_t AddStaticTrigger(PhysicsVector position, Scalar radius);

  /// Adds a force to a movable body, in newtons.
  ///
  /// Forces accumulate until the next @ref Step. Forces applied to static
  /// bodies are ignored.
  void ApplyForce(std::size_t body_index, PhysicsVector force);
  /// Immediately changes a movable body's momentum, in newton-seconds.
  ///
  /// Impulses applied to static bodies are ignored.
  void ApplyImpulse(std::size_t body_index, PhysicsVector impulse);

  /// Advances gravity, positions and contacts by one fixed time step.
  ///
  /// @param delta_time Positive time step, in seconds.
  void Step(Scalar delta_time);

  /// Returns the number of bodies.
  [[nodiscard]] std::size_t body_count() const { return bodies_.size(); }
  /// Returns a body by its index.
  [[nodiscard]] PhysicsBody& body(std::size_t index) {
    return bodies_[index];
  }
  /// Returns a body by its index.
  [[nodiscard]] const PhysicsBody& body(std::size_t index) const {
    return bodies_[index];
  }

 private:
  [[nodiscard]] Scalar InverseMass(const PhysicsBody& body) const;
  void KeepInsideBounds(PhysicsBody& body);
  void ResolveBodyContacts();

  Rect bounds_{};
  bool has_bounds_ = false;
  PhysicsVector gravity_{};
  std::vector<PhysicsBody> bodies_;
};

}  // namespace gpr925

#endif  // GPR925_PHYSICS_WORLD_H_
