#include "gpr925/physics_world.h"

#include <algorithm>

#include "gpr925/scalar.h"

namespace gpr925 {
namespace {

constexpr Scalar kPositionCorrection = Ratio(4, 5);
constexpr Scalar kPenetrationSlop = Ratio(1, 1000);

}  // namespace

std::size_t PhysicsWorld::AddBody(Vec2 position,
                                  Vec2 velocity, Scalar radius,
                                  Scalar mass, Scalar restitution) {
  PhysicsBody body;
  body.position = position;
  body.velocity = velocity;
  body.collider.radius = radius;
  body.mass = mass;
  body.collider.restitution = restitution;
  bodies_.push_back(body);
  return bodies_.size() - 1;
}

std::size_t PhysicsWorld::AddStaticBody(Vec2 position, Scalar radius,
                                        Scalar restitution) {
  PhysicsBody body;
  body.position = position;
  body.collider.radius = radius;
  body.collider.restitution = restitution;
  body.is_static = true;
  bodies_.push_back(body);
  return bodies_.size() - 1;
}

std::size_t PhysicsWorld::AddTriggerBody(Vec2 position,
                                         Vec2 velocity,
                                         Scalar radius) {
  PhysicsBody body;
  body.position = position;
  body.velocity = velocity;
  body.collider.radius = radius;
  body.collider.is_trigger = 1;
  bodies_.push_back(body);
  return bodies_.size() - 1;
}

std::size_t PhysicsWorld::AddStaticTrigger(Vec2 position,
                                           Scalar radius) {
  PhysicsBody body;
  body.position = position;
  body.collider.radius = radius;
  body.is_static = true;
  body.collider.is_trigger = 1;
  bodies_.push_back(body);
  return bodies_.size() - 1;
}

void PhysicsWorld::ApplyForce(std::size_t body_index, Vec2 force) {
  PhysicsBody& target = bodies_[body_index];
  if (!target.is_static) {
    target.force_ += force;
  }
}

void PhysicsWorld::ApplyImpulse(std::size_t body_index,
                                Vec2 impulse) {
  PhysicsBody& target = bodies_[body_index];
  if (!target.is_static) {
    target.velocity += impulse * InverseMass(target);
  }
}

Scalar PhysicsWorld::InverseMass(const PhysicsBody& body) const {
  return body.is_static ? Scalar{} : ToScalar(1) / body.mass;
}

void PhysicsWorld::ApplyGravity(Scalar delta_time) {
  for (PhysicsBody& body : bodies_) {
    if (!body.is_static) {
      body.velocity += gravity_ * delta_time;
    }
  }
}

void PhysicsWorld::Integrate(Scalar delta_time) {
  for (PhysicsBody& body : bodies_) {
    if (body.is_static) {
      body.velocity = {};
      body.force_ = {};
      continue;
    }

    body.velocity += body.force_ * (InverseMass(body) * delta_time);
    body.position += body.velocity * delta_time;
    body.force_ = {};
    if (has_bounds_) {
      KeepInsideBounds(body);
    }
  }
}

void PhysicsWorld::Step(Scalar delta_time) {
  for (PhysicsBody& body : bodies_) {
    body.was_touching_count = body.touching_count;
    body.touching_count = 0;
    body.contact_started_count = 0;
  }
  current_contacts_.clear();

  ApplyGravity(delta_time);
  Integrate(delta_time);
  ResolveBodyContacts();
  NotifyContactEvents();
  previous_contacts_.swap(current_contacts_);
}

void PhysicsWorld::KeepInsideBounds(PhysicsBody& body) {
  if (!has_bounds_) {
    return;
  }

  const Scalar min_x = bounds_.min_bound.x() + body.collider.radius;
  const Scalar max_x = bounds_.max_bound.x() - body.collider.radius;
  const Scalar min_y = bounds_.min_bound.y() + body.collider.radius;
  const Scalar max_y = bounds_.max_bound.y() - body.collider.radius;

  if (min_x > max_x) {
    body.position.x() = bounds_.Center().x();
    body.velocity.x() = Scalar{};
  } else if (body.position.x() < min_x) {
    body.position.x() = min_x;
    if (body.velocity.x() < Scalar{}) {
      body.velocity.x() = -body.velocity.x() * body.collider.restitution;
    }
  } else if (body.position.x() > max_x) {
    body.position.x() = max_x;
    if (body.velocity.x() > Scalar{}) {
      body.velocity.x() = -body.velocity.x() * body.collider.restitution;
    }
  }

  if (min_y > max_y) {
    body.position.y() = bounds_.Center().y();
    body.velocity.y() = Scalar{};
  } else if (body.position.y() < min_y) {
    body.position.y() = min_y;
    if (body.velocity.y() < Scalar{}) {
      body.velocity.y() = -body.velocity.y() * body.collider.restitution;
    }
  } else if (body.position.y() > max_y) {
    body.position.y() = max_y;
    if (body.velocity.y() > Scalar{}) {
      body.velocity.y() = -body.velocity.y() * body.collider.restitution;
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
      Vec2 displacement = second.position - first.position;
      const Scalar distance_squared =
          maths::Vector2<Scalar>::Dot(displacement, displacement);
      const Scalar combined_radius =
          first.collider.radius + second.collider.radius;
      if (distance_squared >= combined_radius * combined_radius) {
        continue;
      }

      const Scalar distance = Sqrt(distance_squared);
      const Vec2 relative_velocity =
          second.velocity - first.velocity;
      const Vec2 normal =
          distance > Scalar{}
              ? displacement * (ToScalar(1) / distance)
              : (relative_velocity.SqrMagnitude() > Scalar{}
                     ? -relative_velocity *
                           (ToScalar(1) / Sqrt(relative_velocity.SqrMagnitude()))
                     : Vec2{ToScalar(1), Scalar{}});
      const Scalar first_inverse_mass = InverseMass(first);
      const Scalar second_inverse_mass = InverseMass(second);
      const Scalar inverse_mass_sum =
          first_inverse_mass + second_inverse_mass;
      ++first.touching_count;
      ++second.touching_count;
      const ContactPair contact{first_index, second_index,
                                first.collider.is_trigger != 0 ||
                                    second.collider.is_trigger != 0};
      if (!std::binary_search(previous_contacts_.begin(),
                              previous_contacts_.end(), contact)) {
        ++first.contact_started_count;
        ++second.contact_started_count;
      }
      current_contacts_.push_back(contact);
      if (contact.is_trigger ||
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
        const Scalar restitution =
            first.collider.restitution < second.collider.restitution
                ? first.collider.restitution
                : second.collider.restitution;
        const Scalar impulse =
            -(ToScalar(1) + restitution) * velocity_along_normal /
            inverse_mass_sum;
        const Vec2 impulse_vector = normal * impulse;
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

void PhysicsWorld::NotifyContactEvents() {
  PhysicsContactListener* const listener = contact_listener_;
  if (listener == nullptr) {
    return;
  }

  for (const ContactPair& contact : previous_contacts_) {
    if (!std::binary_search(current_contacts_.begin(), current_contacts_.end(),
                            contact)) {
      if (contact.is_trigger) {
        listener->OnTriggerExit(contact.first, contact.second);
        listener->OnTriggerExit(contact.second, contact.first);
      } else {
        listener->OnColliderExit(contact.first, contact.second);
        listener->OnColliderExit(contact.second, contact.first);
      }
    }
  }

  for (const ContactPair& contact : current_contacts_) {
    if (!std::binary_search(previous_contacts_.begin(),
                            previous_contacts_.end(), contact)) {
      if (contact.is_trigger) {
        listener->OnTriggerEnter(contact.first, contact.second);
        listener->OnTriggerEnter(contact.second, contact.first);
      } else {
        listener->OnColliderEnter(contact.first, contact.second);
        listener->OnColliderEnter(contact.second, contact.first);
      }
    }
  }
}

}  // namespace gpr925
