#include "gpr925/physics_world.h"

#include "gpr925/scalar.h"

namespace gpr925 {
namespace {

constexpr Scalar kPositionCorrection = Ratio(4, 5);
constexpr Scalar kPenetrationSlop = Ratio(1, 1000);

}  // namespace

std::size_t PhysicsWorld::AddBody(PhysicsVector position,
                                  PhysicsVector velocity, Scalar radius,
                                  Scalar mass, Scalar restitution) {
  PhysicsBody body;
  body.position = position;
  body.velocity = velocity;
  body.radius = radius;
  body.mass = mass;
  body.restitution = restitution;
  bodies_.push_back(body);
  return bodies_.size() - 1;
}

std::size_t PhysicsWorld::AddStaticBody(PhysicsVector position, Scalar radius,
                                        Scalar restitution) {
  PhysicsBody body;
  body.position = position;
  body.radius = radius;
  body.restitution = restitution;
  body.is_static = true;
  bodies_.push_back(body);
  return bodies_.size() - 1;
}

std::size_t PhysicsWorld::AddTriggerBody(PhysicsVector position,
                                         PhysicsVector velocity,
                                         Scalar radius) {
  PhysicsBody body;
  body.position = position;
  body.velocity = velocity;
  body.radius = radius;
  body.is_trigger = true;
  bodies_.push_back(body);
  return bodies_.size() - 1;
}

std::size_t PhysicsWorld::AddStaticTrigger(PhysicsVector position,
                                           Scalar radius) {
  PhysicsBody body;
  body.position = position;
  body.radius = radius;
  body.is_static = true;
  body.is_trigger = true;
  bodies_.push_back(body);
  return bodies_.size() - 1;
}

void PhysicsWorld::ApplyForce(std::size_t body_index, PhysicsVector force) {
  PhysicsBody& target = bodies_[body_index];
  if (!target.is_static) {
    target.force_ += force;
  }
}

void PhysicsWorld::ApplyImpulse(std::size_t body_index,
                                PhysicsVector impulse) {
  PhysicsBody& target = bodies_[body_index];
  if (!target.is_static) {
    target.velocity += impulse * InverseMass(target);
  }
}

Scalar PhysicsWorld::InverseMass(const PhysicsBody& body) const {
  return body.is_static ? Scalar{} : ToScalar(1) / body.mass;
}

void PhysicsWorld::Step(Scalar delta_time) {
  for (PhysicsBody& body : bodies_) {
    body.contact_started = false;
    body.touching_ = false;
    if (body.is_static) {
      body.velocity = {};
      body.force_ = {};
      continue;
    }

    body.velocity +=
        (gravity_ + body.force_ * InverseMass(body)) * delta_time;
    body.position += body.velocity * delta_time;
    body.force_ = {};
    if (has_bounds_) {
      KeepInsideBounds(body);
    }
  }

  ResolveBodyContacts();

  for (PhysicsBody& body : bodies_) {
    body.contact_started = body.touching_ && !body.was_touching_;
    body.was_touching_ = body.touching_;
  }
}

void PhysicsWorld::KeepInsideBounds(PhysicsBody& body) {
  if (!has_bounds_) {
    return;
  }

  const Scalar min_x = bounds_.min_bound.x + body.radius;
  const Scalar max_x = bounds_.max_bound.x - body.radius;
  const Scalar min_y = bounds_.min_bound.y + body.radius;
  const Scalar max_y = bounds_.max_bound.y - body.radius;

  if (min_x > max_x) {
    body.position.x() = bounds_.Center().x;
    body.velocity.x() = Scalar{};
  } else if (body.position.x() < min_x) {
    body.position.x() = min_x;
    if (body.velocity.x() < Scalar{}) {
      body.velocity.x() = -body.velocity.x() * body.restitution;
    }
  } else if (body.position.x() > max_x) {
    body.position.x() = max_x;
    if (body.velocity.x() > Scalar{}) {
      body.velocity.x() = -body.velocity.x() * body.restitution;
    }
  }

  if (min_y > max_y) {
    body.position.y() = bounds_.Center().y;
    body.velocity.y() = Scalar{};
  } else if (body.position.y() < min_y) {
    body.position.y() = min_y;
    if (body.velocity.y() < Scalar{}) {
      body.velocity.y() = -body.velocity.y() * body.restitution;
    }
  } else if (body.position.y() > max_y) {
    body.position.y() = max_y;
    if (body.velocity.y() > Scalar{}) {
      body.velocity.y() = -body.velocity.y() * body.restitution;
    }
  }
}

void PhysicsWorld::ResolveBodyContacts() {
  for (std::size_t first_index = 0; first_index < bodies_.size();
       ++first_index) {
    PhysicsBody& first = bodies_[first_index];
    for (std::size_t second_index = first_index + 1;
         second_index < bodies_.size(); ++second_index) {
      PhysicsBody& second = bodies_[second_index];
      PhysicsVector displacement = second.position - first.position;
      const Scalar distance_squared =
          maths::Vector2<Scalar>::Dot(displacement, displacement);
      const Scalar combined_radius = first.radius + second.radius;
      if (distance_squared >= combined_radius * combined_radius) {
        continue;
      }

      const Scalar distance = Sqrt(distance_squared);
      const PhysicsVector relative_velocity =
          second.velocity - first.velocity;
      const PhysicsVector normal =
          distance > Scalar{}
              ? displacement * (ToScalar(1) / distance)
              : (relative_velocity.SqrMagnitude() > Scalar{}
                     ? -relative_velocity *
                           (ToScalar(1) / Sqrt(relative_velocity.SqrMagnitude()))
                     : PhysicsVector{ToScalar(1), Scalar{}});
      const Scalar first_inverse_mass = InverseMass(first);
      const Scalar second_inverse_mass = InverseMass(second);
      const Scalar inverse_mass_sum =
          first_inverse_mass + second_inverse_mass;
      first.touching_ = true;
      second.touching_ = true;
      if (first.is_trigger || second.is_trigger ||
          inverse_mass_sum == Scalar{}) {
        continue;
      }

      const Scalar penetration = combined_radius - distance;
      const Scalar correction =
          (penetration > kPenetrationSlop
               ? (penetration - kPenetrationSlop) * kPositionCorrection
               : Scalar{}) /
          inverse_mass_sum;
      first.position -= normal * (correction * first_inverse_mass);
      second.position += normal * (correction * second_inverse_mass);

      const Scalar velocity_along_normal =
          maths::Vector2<Scalar>::Dot(relative_velocity, normal);
      if (velocity_along_normal < Scalar{}) {
        const Scalar restitution = first.restitution < second.restitution
                                       ? first.restitution
                                       : second.restitution;
        const Scalar impulse =
            -(ToScalar(1) + restitution) * velocity_along_normal /
            inverse_mass_sum;
        const PhysicsVector impulse_vector = normal * impulse;
        first.velocity -= impulse_vector * first_inverse_mass;
        second.velocity += impulse_vector * second_inverse_mass;
      }
    }
  }

  if (has_bounds_) {
    for (PhysicsBody& body : bodies_) {
      if (!body.is_static) {
        KeepInsideBounds(body);
      }
    }
  }
}

}  // namespace gpr925
