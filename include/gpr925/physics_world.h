#ifndef GPR925_PHYSICS_WORLD_H_
#define GPR925_PHYSICS_WORLD_H_

#include <cstddef>
#include <vector>

#include "gpr925/engine_lib_number_traits.h"
#include "gpr925/vec2.h"

#include "vector2.h"

namespace gpr925 {

/// Vector type used by EngineLib's math and physics algorithms.
using Vec2 = maths::Vector2<Scalar>;

/// Circular shape and contact behavior attached to a physics body.
struct Collider {
  Scalar radius{};                    ///< Collision radius, in meters.
  Scalar restitution = ToScalar(1);   ///< Bounciness from zero to one.
  int is_trigger = 0;                 ///< Nonzero for overlap-only contacts.
};

/// Receives pairwise contact callbacks from a @ref PhysicsWorld.
///
/// Each callback is called once for each participant, with that body's index
/// first and the other body's index second. Callbacks run during @ref Step;
/// they can apply forces or impulses, but must defer adding or removing bodies
/// until the step has finished.
class PhysicsContactListener {
 public:
  virtual ~PhysicsContactListener() = default;
  virtual void OnTriggerEnter(std::size_t, std::size_t) {}
  virtual void OnTriggerExit(std::size_t, std::size_t) {}
  virtual void OnColliderEnter(std::size_t, std::size_t) {}
  virtual void OnColliderExit(std::size_t, std::size_t) {}
};

/// A circular rigid body simulated by @ref PhysicsWorld.
class PhysicsBody {
 public:
  Vec2 position{};       ///< Center of the body, in meters.
  Vec2 velocity{};       ///< Velocity, in meters per second.
  Collider collider{};             ///< Shape and trigger/collider settings.
  Scalar mass = ToScalar(1);      ///< Mass; ignored for static bodies.
  bool is_static = false;          ///< Static bodies do not move.
  std::size_t contact_started_count = 0;  ///< New contacts during latest step.
  std::size_t touching_count = 0;        ///< Current overlapping bodies.
  std::size_t was_touching_count = 0;     ///< Overlapping bodies last step.

 private:
  Vec2 force_{};

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
  void SetGravity(Vec2 gravity) { gravity_ = gravity; }
  /// Sets the non-owning recipient of trigger and collider enter/exit events.
  ///
  /// The listener must remain alive until it is unregistered or the world is
  /// destroyed.
  void SetContactListener(PhysicsContactListener* listener) {
    contact_listener_ = listener;
  }
  /// Reserves body storage; call during setup to avoid simulation-time growth.
  void Reserve(std::size_t body_count) { bodies_.reserve(body_count); }
  /// Removes all bodies and contact history while retaining allocated storage.
  void Clear() {
    bodies_.clear();
    previous_contacts_.clear();
    current_contacts_.clear();
  }

  /// Adds a movable circular body with a solid collider.
  ///
  /// @param position Center in meters.
  /// @param velocity In meters per second.
  /// @param radius Positive collision radius in meters.
  /// @param mass Positive mass; the world uses its reciprocal.
  /// @param restitution Bounciness from zero (no bounce) to one (elastic).
  /// @return Index used to retrieve the body from @ref body.
  ///
  /// Radius and mass must be positive. Restitution must be in [0, 1].
  std::size_t AddBody(Vec2 position, Vec2 velocity,
                      Scalar radius, Scalar mass,
                      Scalar restitution = ToScalar(1));

  /// Adds an immovable circular body with a solid collider, such as a wall.
  ///
  /// @param position Center in meters.
  /// @param radius Positive collision radius in meters.
  /// @param restitution Bounciness from zero (no bounce) to one (elastic).
  /// @return Index used to retrieve the body from @ref body.
  std::size_t AddStaticBody(Vec2 position, Scalar radius,
                            Scalar restitution = ToScalar(1));

  /// Adds a movable circular trigger that reports enter and exit events
  /// without resolving overlaps or changing either body's velocity.
  ///
  /// @param position Center in meters.
  /// @param velocity In meters per second.
  /// @param radius Positive trigger radius in meters.
  /// @return Index used to retrieve the body from @ref body.

  std::size_t AddTriggerBody(Vec2 position, Vec2 velocity,
                             Scalar radius);

  /// Adds an immovable circular trigger volume that reports enter and exit.
  ///
  /// @param position Center in meters.
  /// @param radius Positive trigger radius in meters.
  /// @return Index used to retrieve the body from @ref body.
  std::size_t AddStaticTrigger(Vec2 position, Scalar radius);

  /// Adds a force to a movable body, in newtons.
  ///
  /// Forces accumulate until the next @ref Step. Forces applied to static
  /// bodies are ignored.
  void ApplyForce(std::size_t body_index, Vec2 force);
  /// Immediately changes a movable body's momentum, in newton-seconds.
  ///
  /// Impulses applied to static bodies are ignored.
  void ApplyImpulse(std::size_t body_index, Vec2 impulse);

  /// Applies the world's gravity to movable body velocities.
  ///
  /// @param delta_time Time interval, in seconds.
  void ApplyGravity(Scalar delta_time);

  /// Integrates body forces and positions over a time interval.
  ///
  /// Accumulated forces are cleared after integration.
  /// @param delta_time Time interval, in seconds.
  void Integrate(Scalar delta_time);

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
  struct ContactPair {
    std::size_t first;
    std::size_t second;
    bool is_trigger;

    bool operator<(const ContactPair& other) const {
      if (first != other.first) {
        return first < other.first;
      }
      if (second != other.second) {
        return second < other.second;
      }
      return is_trigger < other.is_trigger;
    }

    bool operator==(const ContactPair& other) const = default;
  };

  [[nodiscard]] Scalar InverseMass(const PhysicsBody& body) const;
  void KeepInsideBounds(PhysicsBody& body);
  void ResolveBodyContacts();
  void NotifyContactEvents();

  Rect bounds_{};
  bool has_bounds_ = false;
  Vec2 gravity_{};
  PhysicsContactListener* contact_listener_ = nullptr;
  std::vector<PhysicsBody> bodies_;
  std::vector<ContactPair> previous_contacts_;
  std::vector<ContactPair> current_contacts_;
};

}  // namespace gpr925

#endif  // GPR925_PHYSICS_WORLD_H_
