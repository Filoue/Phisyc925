#include "main/samples/05_physics_world.h"

#include "gpr925/scalar.h"

namespace gpr925 {

void PhysicsWorldSample::Begin(const SampleContext& context) {
  world_.Clear();
  world_.Reserve(3);
  world_.SetBounds(context.world_bounds);
  world_.SetGravity({Scalar{}, Ratio(981, 100)});
  world_.SetContactListener(this);

  const Vec2 center = context.world_bounds.Center();
  first_body_ = world_.AddBody(
      {center.x - Ratio(1, 2), center.y - ToScalar(1)},
      {ToScalar(1), Scalar{}}, Ratio(2, 5), ToScalar(1), Ratio(3, 4));
  second_body_ = world_.AddBody(
      {center.x + Ratio(1, 2), center.y - ToScalar(1)},
      {-ToScalar(1), Scalar{}}, Ratio(2, 5), ToScalar(1), Ratio(1, 2));
  trigger_body_ = world_.AddStaticTrigger(
      {center.x + ToScalar(2), center.y + Ratio(3, 2)}, Ratio(1, 2));
}

void PhysicsWorldSample::FixedUpdate(const Input& input) {
  trigger_entered_ = false;
  trigger_exited_ = false;
  collider_entered_ = false;
  collider_exited_ = false;
  if (input.WasPressed(Button::kA)) {
    world_.ApplyImpulse(first_body_, {Scalar{}, -ToScalar(2)});
  }
  world_.Step(kFixedDeltaTime);
}

void PhysicsWorldSample::Draw(DrawList& draw_list) const {
  for (std::size_t i = 0; i < world_.body_count(); ++i) {
    const PhysicsBody& body = world_.body(i);
    ColorId color = sample_color::kBlue;
    if (i == trigger_body_) {
      color = trigger_entered_ ? sample_color::kYellow
                               : (trigger_exited_ ? sample_color::kOrange
                                                  : sample_color::kTeal);
    } else if (body.contact_started_count > 0 || collider_entered_) {
      color = sample_color::kYellow;
    } else if (collider_exited_) {
      color = sample_color::kOrange;
    }
    draw_list.circles.push_back(
        {{body.position.x(), body.position.y()}, body.collider.radius, color});
  }
}

void PhysicsWorldSample::End() {
  world_.SetContactListener(nullptr);
  world_.Clear();
  first_body_ = 0;
  second_body_ = 0;
  trigger_body_ = 0;
}

void PhysicsWorldSample::OnTriggerEnter(std::size_t body_index,
                                        std::size_t other_body_index) {
  if (body_index != trigger_body_) {
    return;
  }

  trigger_entered_ = true;
  world_.ApplyImpulse(other_body_index, {Scalar{}, -ToScalar(1)});
}

void PhysicsWorldSample::OnTriggerExit(std::size_t, std::size_t) {
  trigger_exited_ = true;
}

void PhysicsWorldSample::OnColliderEnter(std::size_t, std::size_t) {
  collider_entered_ = true;
}

void PhysicsWorldSample::OnColliderExit(std::size_t, std::size_t) {
  collider_exited_ = true;
}

}  // namespace gpr925
