#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "gpr925/scalar.h"

#include "main/samples/01_hello.h"
#include "main/samples/02_planet.h"
#include "main/samples/03_planetFourVec2.h"
#include "main/samples/04_trigger.h"
#include "main/samples/05_Collision.h"
#include "main/samples/sample.h"
#include "main/samples/sample_config.h"

namespace gpr925 {
namespace {

constexpr int kStepCount = 600;

bool IsInside(Vec2 point, const Rect& bounds, Scalar margin) {
  return point.x() >= bounds.min_bound.x() - margin &&
         point.x() <= bounds.max_bound.x() + margin &&
         point.y() >= bounds.min_bound.y() - margin &&
         point.y() <= bounds.max_bound.y() + margin;
}

TEST(SampleRegistry, StartsWithNoSampleSelected) {
  SampleRegistry registry;
  EXPECT_GE(registry.size(), 1u);
  EXPECT_EQ(registry.current_index(), SampleRegistry::kNone);
  EXPECT_EQ(registry.current(), nullptr);

  DrawList draw_list;
  registry.FixedUpdate({});
  registry.Draw(draw_list);
  EXPECT_TRUE(draw_list.empty());
}

// Every registered sample runs, draws something and stays within the DrawList
// capacities reserved by the hosts
TEST(SampleRegistry, EverySampleRunsAndDraws) {
  SampleRegistry registry;
  const SampleContext context{sample_config::kWorldBounds};
  DrawList draw_list;
  for (std::size_t index = 0; index < registry.size(); ++index) {
    SCOPED_TRACE(registry.name(index));
    registry.Select(index, context);
    EXPECT_EQ(registry.current_index(), index);
    for (int step = 0; step < kStepCount; ++step) {
      registry.FixedUpdate({});
    }
    registry.Draw(draw_list);
    EXPECT_FALSE(draw_list.empty());
    EXPECT_LE(draw_list.circles.size(), sample_config::kMaxDrawCircles);
    EXPECT_LE(draw_list.boxes.size(), sample_config::kMaxDrawBoxes);
    EXPECT_LE(draw_list.lines.size(), sample_config::kMaxDrawLines);
  }
}

TEST(HelloSample, BallsStayInsideTheWorld) {
  const SampleContext context{sample_config::kWorldBounds};
  HelloSample sample;
  sample.Begin(context);
  for (int step = 0; step < kStepCount; ++step) {
    sample.FixedUpdate({});
  }
  DrawList draw_list;
  sample.Draw(draw_list);
  // The balls and the player
  ASSERT_EQ(draw_list.circles.size(), sample_config::kHelloBallCount + 1);
  for (const DrawCircle& circle : draw_list.circles) {
    // A ball turns around after it crossed the border: allow one step of
    // travel
    EXPECT_TRUE(IsInside(circle.center, context.world_bounds, Ratio(1, 10)));
  }
  sample.End();
}

TEST(HelloSample, RestartGivesTheSameScene) {
  const SampleContext context{sample_config::kWorldBounds};
  HelloSample sample;
  DrawList first;
  DrawList second;
  sample.Begin(context);
  sample.Draw(first);
  sample.End();
  sample.Begin(context);
  sample.Draw(second);
  sample.End();
  ASSERT_EQ(first.circles.size(), second.circles.size());
  for (std::size_t i = 0; i < first.circles.size(); ++i) {
    EXPECT_EQ(first.circles[i].center, second.circles[i].center);
    EXPECT_EQ(first.circles[i].color, second.circles[i].color);
  }
}

TEST(HelloSample, DpadMovesThePlayer) {
  const SampleContext context{sample_config::kWorldBounds};
  HelloSample sample;
  sample.Begin(context);
  const Vec2 start = sample.player_position();

  const Input right = Input::FromHeld(ButtonMask(Button::kRight), 0);
  for (int step = 0; step < 30; ++step) {
    sample.FixedUpdate(right);
  }
  EXPECT_GT(sample.player_position().x(), start.x());
  EXPECT_EQ(sample.player_position().y(), start.y());

  const Input up = Input::FromHeld(ButtonMask(Button::kUp), 0);
  for (int step = 0; step < 30; ++step) {
    sample.FixedUpdate(up);
  }
  EXPECT_LT(sample.player_position().y(), start.y());
  sample.End();
}

TEST(HelloSample, APressChangesThePlayerColorOnce) {
  const SampleContext context{sample_config::kWorldBounds};
  HelloSample sample;
  sample.Begin(context);
  DrawList draw_list;
  sample.Draw(draw_list);
  const ColorId initial = draw_list.circles.back().color;

  const std::uint8_t a = ButtonMask(Button::kA);
  sample.FixedUpdate(Input::FromHeld(a, 0));
  // Still down on the next steps: held, but not pressed again
  sample.FixedUpdate(Input::FromHeld(a, a));
  sample.FixedUpdate(Input::FromHeld(a, a));

  draw_list.Clear();
  sample.Draw(draw_list);
  EXPECT_EQ(draw_list.circles.back().color, initial + 1);
  sample.End();
}

TEST(PlanetSample, Draws128ScatteredPlanetsWithoutCenterOrOrbitLines) {
  const SampleContext context{sample_config::kWorldBounds};
  PlanetSample sample;
  sample.Begin(context);
  DrawList draw_list;
  sample.Draw(draw_list);
  ASSERT_EQ(draw_list.circles.size(), 128u);
  EXPECT_TRUE(draw_list.lines.empty());
  const Vec2 first_position = draw_list.circles[0].center;
  const Vec2 second_position = draw_list.circles[1].center;
  EXPECT_NE(first_position, second_position);
  EXPECT_NE((first_position - context.world_bounds.Center()).Magnitude(),
            (second_position - context.world_bounds.Center()).Magnitude());
  sample.End();
}

TEST(PlanetSample, PlanetsOrbitAtDifferentDistances) {
  const SampleContext context{sample_config::kWorldBounds};
  PlanetSample sample;
  sample.Begin(context);
  DrawList initial;
  DrawList moved;
  sample.Draw(initial);
  for (int step = 0; step < 60; ++step) {
    sample.FixedUpdate({});
  }
  sample.Draw(moved);
  for (std::size_t i = 0; i < initial.circles.size(); ++i) {
    EXPECT_NE(initial.circles[i].center, moved.circles[i].center);
  }
  sample.End();
}

TEST(PlanetSample, APressPausesAndBRestartsThePlanets) {
  const SampleContext context{sample_config::kWorldBounds};
  PlanetSample sample;
  sample.Begin(context);
  DrawList initial;
  DrawList after_pause;
  DrawList after_reset;
  sample.Draw(initial);
  const std::uint8_t a = ButtonMask(Button::kA);
  const Input press_a = Input::FromHeld(a, 0);
  const Input held_a = Input::FromHeld(a, a);
  sample.FixedUpdate(press_a);
  for (int step = 0; step < 30; ++step) {
    sample.FixedUpdate(held_a);
  }
  sample.Draw(after_pause);
  EXPECT_EQ(initial.circles[0].center, after_pause.circles[0].center);

  sample.FixedUpdate(Input::FromHeld(ButtonMask(Button::kB), 0));
  sample.Draw(after_reset);
  EXPECT_EQ(initial.circles[0].center, after_reset.circles[0].center);
  sample.End();
}

TEST(PlanetFourVec2, Draws128PlanetsAndMovesThemWithGravity) {
  const SampleContext context{sample_config::kWorldBounds};
  PlanetFourVec2 sample;
  sample.Begin(context);
  DrawList initial;
  DrawList moved;
  sample.Draw(initial);
  ASSERT_EQ(initial.circles.size(), 128u);
  EXPECT_TRUE(initial.lines.empty());

  sample.FixedUpdate({});
  sample.Draw(moved);
  ASSERT_EQ(moved.circles.size(), initial.circles.size());
  for (std::size_t i = 0; i < initial.circles.size(); ++i) {
    EXPECT_NE(initial.circles[i].center, moved.circles[i].center);
  }
  sample.End();
}

TEST(PlanetFourVec2, ResetRecreatesTheSameInitialState) {
  const SampleContext context{sample_config::kWorldBounds};
  PlanetFourVec2 sample;
  sample.Begin(context);
  DrawList initial;
  DrawList reset;
  sample.Draw(initial);
  sample.FixedUpdate(Input::FromHeld(ButtonMask(Button::kB), 0));
  sample.Draw(reset);
  ASSERT_EQ(reset.circles.size(), initial.circles.size());
  for (std::size_t i = 0; i < initial.circles.size(); ++i) {
    EXPECT_EQ(initial.circles[i].center, reset.circles[i].center);
    EXPECT_EQ(initial.circles[i].color, reset.circles[i].color);
  }
  sample.End();
}

TEST(PlanetFourVec2, PlanetsStayWithinTheWorldDuringGravitySimulation) {
  const SampleContext context{sample_config::kWorldBounds};
  PlanetFourVec2 sample;
  sample.Begin(context);
  for (int step = 0; step < kStepCount; ++step) {
    sample.FixedUpdate({});
  }
  DrawList draw_list;
  sample.Draw(draw_list);
  ASSERT_EQ(draw_list.circles.size(), 128u);
  for (std::size_t i = 0; i < draw_list.circles.size(); ++i) {
    const Vec2 position = draw_list.circles[i].center;
    EXPECT_TRUE(IsInside(position, context.world_bounds, Ratio(1, 10)))
        << "planet " << i << " at " << ToFloat(position.x()) << ", "
        << ToFloat(position.y());
  }
  sample.End();
}

TEST(PhysicsWorld, ResolvesBallCollisionsAndReportsNewContacts) {
  PhysicsWorld world;
  world.SetBounds({{Scalar{}, Scalar{}},
                   {ToScalar(10), ToScalar(10)}});
  world.AddBody({ToScalar(4), ToScalar(5)}, {ToScalar(1), Scalar{}},
                Ratio(1, 2), ToScalar(1));
  world.AddBody({ToScalar(6), ToScalar(5)}, {-ToScalar(1), Scalar{}},
                Ratio(1, 2), ToScalar(1));

  world.Step(Ratio(1, 2));
  world.Step(Ratio(1, 2));
  ASSERT_EQ(world.body_count(), 2u);
  EXPECT_EQ(world.body(0).contact_started_count, 1u);
  EXPECT_EQ(world.body(1).contact_started_count, 1u);
  EXPECT_LT(world.body(0).velocity.x(), Scalar{});
  EXPECT_GT(world.body(1).velocity.x(), Scalar{});

  world.Step(Ratio(1, 60));
  EXPECT_EQ(world.body(0).contact_started_count, 0u);
  EXPECT_EQ(world.body(1).contact_started_count, 0u);
}

TEST(PhysicsWorld, GravityAndBoundsAffectBodyMotion) {
  PhysicsWorld world;
  world.SetBounds({{Scalar{}, Scalar{}},
                   {ToScalar(10), ToScalar(10)}});
  world.SetGravity({Scalar{}, ToScalar(1)});
  world.AddBody({ToScalar(5), Ratio(97, 10)},
                {Scalar{}, ToScalar(2)}, Ratio(1, 2), ToScalar(1));

  world.Step(Ratio(1, 10));

  EXPECT_EQ(world.body(0).position.y(), Ratio(19, 2));
  EXPECT_LT(world.body(0).velocity.y(), Scalar{});
}

TEST(PhysicsWorld, GravityAndIntegrationCanBeAppliedSeparately) {
  PhysicsWorld world;
  world.SetGravity({Scalar{}, ToScalar(2)});
  const std::size_t body =
      world.AddBody({ToScalar(1), Scalar{}}, {ToScalar(1), Scalar{}},
                    Ratio(1, 2), ToScalar(1));

  world.ApplyGravity(Ratio(1, 2));
  EXPECT_EQ(world.body(body).velocity.y(), ToScalar(1));
  EXPECT_EQ(world.body(body).position.y(), Scalar{});

  world.Integrate(Ratio(1, 2));
  EXPECT_EQ(world.body(body).position.x(), Ratio(3, 2));
  EXPECT_EQ(world.body(body).position.y(), Ratio(1, 2));
}

TEST(PhysicsWorld, ForcesAccumulateUntilTheNextStep) {
  PhysicsWorld world;
  const std::size_t body =
      world.AddBody({}, {}, Ratio(1, 2), ToScalar(2));
  world.ApplyForce(body, {ToScalar(4), Scalar{}});
  world.Step(Ratio(1, 2));
  EXPECT_EQ(world.body(body).velocity.x(), ToScalar(1));
  EXPECT_EQ(world.body(body).position.x(), Ratio(1, 2));

  world.Step(Ratio(1, 2));
  EXPECT_EQ(world.body(body).velocity.x(), ToScalar(1));
  EXPECT_EQ(world.body(body).position.x(), ToScalar(1));
}

TEST(PhysicsWorld, ImpulsesAndStaticBodiesWorkWithoutWorldBounds) {
  PhysicsWorld world;
  const std::size_t obstacle =
      world.AddStaticBody({ToScalar(6), ToScalar(5)}, ToScalar(1));
  const std::size_t ball =
      world.AddBody({ToScalar(3), ToScalar(5)}, {ToScalar(2), Scalar{}},
                    ToScalar(1), ToScalar(1));
  world.ApplyImpulse(ball, {ToScalar(1), Scalar{}});
  world.ApplyImpulse(obstacle, {ToScalar(100), Scalar{}});
  world.Step(ToScalar(1));

  EXPECT_EQ(world.body(obstacle).position.x(), ToScalar(6));
  EXPECT_EQ(world.body(obstacle).velocity.x(), Scalar{});
  EXPECT_LT(world.body(ball).velocity.x(), Scalar{});
  EXPECT_EQ(world.body(obstacle).contact_started_count, 1u);
  EXPECT_EQ(world.body(ball).contact_started_count, 1u);
}

TEST(PhysicsWorld, BallTriggersDetectEachOtherWithoutChangingVelocity) {
  PhysicsWorld world;
  const std::size_t first =
      world.AddTriggerBody({Scalar{}, ToScalar(5)}, {ToScalar(2), Scalar{}},
                           ToScalar(1));
  const std::size_t second =
      world.AddTriggerBody({ToScalar(3), ToScalar(5)}, {-ToScalar(2), Scalar{}},
                           ToScalar(1));

  world.Step(Ratio(1, 2));

  EXPECT_EQ(world.body(first).position.x(), ToScalar(1));
  EXPECT_EQ(world.body(second).position.x(), ToScalar(2));
  EXPECT_EQ(world.body(first).velocity.x(), ToScalar(2));
  EXPECT_EQ(world.body(second).velocity.x(), -ToScalar(2));
  EXPECT_EQ(world.body(first).contact_started_count, 1u);
  EXPECT_EQ(world.body(second).contact_started_count, 1u);
}

TEST(PhysicsWorld, NotifiesColliderAndTriggerEnterAndExit) {
  class Listener : public PhysicsContactListener {
   public:
    PhysicsWorld* world = nullptr;
    std::size_t trigger_body = 0;

    void OnTriggerEnter(std::size_t body, std::size_t other) override {
      trigger_enters.emplace_back(body, other);
      if (body == trigger_body) {
        world->ApplyImpulse(other, {Scalar{}, -ToScalar(2)});
      }
    }
    void OnTriggerExit(std::size_t body, std::size_t other) override {
      trigger_exits.emplace_back(body, other);
    }
    void OnColliderEnter(std::size_t body, std::size_t other) override {
      collider_enters.emplace_back(body, other);
    }
    void OnColliderExit(std::size_t body, std::size_t other) override {
      collider_exits.emplace_back(body, other);
    }

    std::vector<std::pair<std::size_t, std::size_t>> trigger_enters;
    std::vector<std::pair<std::size_t, std::size_t>> trigger_exits;
    std::vector<std::pair<std::size_t, std::size_t>> collider_enters;
    std::vector<std::pair<std::size_t, std::size_t>> collider_exits;
  };

  Listener listener;
  PhysicsWorld world;
  world.SetContactListener(&listener);
  const std::size_t solid =
      world.AddBody({}, {}, ToScalar(1), ToScalar(1));
  const std::size_t other_solid =
      world.AddBody({Ratio(3, 2), Scalar{}}, {}, ToScalar(1), ToScalar(1));
  EXPECT_EQ(world.body(solid).collider.is_trigger, 0);

  world.Step(Ratio(1, 60));
  ASSERT_EQ(listener.collider_enters.size(), 2u);
  EXPECT_EQ(listener.collider_enters[0],
            (std::pair<std::size_t, std::size_t>{solid, other_solid}));
  EXPECT_EQ(listener.collider_enters[1],
            (std::pair<std::size_t, std::size_t>{other_solid, solid}));

  world.body(other_solid).position.x() = ToScalar(5);
  world.Step(Ratio(1, 60));
  ASSERT_EQ(listener.collider_exits.size(), 2u);

  world.Clear();
  listener.collider_enters.clear();
  listener.collider_exits.clear();
  const std::size_t trigger =
      world.AddStaticTrigger({}, ToScalar(1));
  const std::size_t entering_body =
      world.AddBody({Ratio(3, 2), Scalar{}}, {}, ToScalar(1), ToScalar(1));
  EXPECT_EQ(world.body(trigger).collider.is_trigger, 1);
  listener.world = &world;
  listener.trigger_body = trigger;

  world.Step(Ratio(1, 60));
  ASSERT_EQ(listener.trigger_enters.size(), 2u);
  EXPECT_EQ(listener.trigger_enters[0],
            (std::pair<std::size_t, std::size_t>{trigger, entering_body}));
  EXPECT_EQ(world.body(entering_body).velocity.y(), -ToScalar(2));

  world.body(entering_body).position.x() = ToScalar(5);
  world.Step(Ratio(1, 60));
  ASSERT_EQ(listener.trigger_exits.size(), 2u);
  EXPECT_EQ(listener.trigger_exits[0],
            (std::pair<std::size_t, std::size_t>{trigger, entering_body}));
}

TEST(PhysicsWorld, CountsSimultaneousAndNewContactsIndividually) {
  PhysicsWorld world;
  const std::size_t center =
      world.AddTriggerBody({ToScalar(5), ToScalar(5)}, {}, ToScalar(1));
  const std::size_t left =
      world.AddStaticTrigger({Ratio(7, 2), ToScalar(5)}, ToScalar(1));
  world.AddStaticTrigger({Ratio(13, 2), ToScalar(5)}, ToScalar(1));
  const std::size_t arriving =
      world.AddStaticTrigger({ToScalar(9), ToScalar(5)}, ToScalar(1));

  world.Step(Ratio(1, 60));
  EXPECT_EQ(world.body(center).touching_count, 2u);
  EXPECT_EQ(world.body(center).contact_started_count, 2u);

  world.body(left).position.x() = ToScalar(1);
  world.body(arriving).position.x() = Ratio(9, 2);
  world.Step(Ratio(1, 60));
  EXPECT_EQ(world.body(center).was_touching_count, 2u);
  EXPECT_EQ(world.body(center).touching_count, 2u);
  EXPECT_EQ(world.body(center).contact_started_count, 1u);
}

TEST(PhysicsWorld, TriggerBodiesBounceAtWorldBoundsOnly) {
  PhysicsWorld world;
  world.SetBounds({{Scalar{}, Scalar{}},
                   {ToScalar(10), ToScalar(10)}});
  const std::size_t trigger =
      world.AddStaticTrigger({ToScalar(9), ToScalar(5)}, ToScalar(1));
  const std::size_t ball = world.AddTriggerBody(
      {Ratio(17, 2), ToScalar(5)}, {ToScalar(2), Scalar{}}, Ratio(1, 2));

  world.Step(Ratio(1, 2));
  EXPECT_EQ(world.body(ball).position.x(), Ratio(19, 2));
  EXPECT_EQ(world.body(ball).velocity.x(), ToScalar(2));
  EXPECT_EQ(world.body(trigger).contact_started_count, 1u);
  EXPECT_EQ(world.body(ball).contact_started_count, 1u);

  world.Step(Ratio(1, 60));
  EXPECT_EQ(world.body(ball).position.x(), Ratio(19, 2));
  EXPECT_LT(world.body(ball).velocity.x(), Scalar{});
}

TEST(TriggerSample, BallsChangeColorWhenTheyTriggerEachOther) {
  const SampleContext context{sample_config::kWorldBounds};
  TriggerSample sample;
  sample.Begin(context);
  DrawList initial;
  DrawList stepped;
  DrawList moved;
  sample.Draw(initial);
  ASSERT_EQ(initial.circles.size(), sample_config::kTriggerBallCount);

  for (int step = 0; step < 10; ++step) {
    sample.FixedUpdate({});
  }
  sample.Draw(stepped);
  for (int step = 10; step < kStepCount; ++step) {
    sample.FixedUpdate({});
  }
  sample.Draw(moved);
  ASSERT_EQ(moved.circles.size(), initial.circles.size());
  bool color_changed = false;
  for (std::size_t i = 0; i < initial.circles.size(); ++i) {
    color_changed |= initial.circles[i].color != moved.circles[i].color;
  }
  EXPECT_TRUE(color_changed);
  sample.End();
}

TEST(CollisionSample, BallsFallAndAreGreenOnlyWhileTouching) {
  const SampleContext context{sample_config::kWorldBounds};
  CollisionSample sample;
  sample.Begin(context);
  DrawList initial;
  DrawList current;
  sample.Draw(initial);

  ASSERT_EQ(initial.circles.size(), sample_config::kCollisionBallCount);
  for (const DrawCircle& ball : initial.circles) {
    EXPECT_EQ(ball.color, sample_color::kRed);
  }
  EXPECT_NE(initial.circles[0].radius, initial.circles[1].radius);

  std::vector<bool> seen_green(initial.circles.size(), false);
  bool saw_green = false;
  bool saw_red_after_green = false;
  for (int step = 0; step < 600; ++step) {
    sample.FixedUpdate({});
    current.Clear();
    sample.Draw(current);
    ASSERT_EQ(current.circles.size(), initial.circles.size());
    for (std::size_t i = 0; i < current.circles.size(); ++i) {
      if (current.circles[i].color == sample_color::kGreen) {
        seen_green[i] = true;
        saw_green = true;
      } else if (seen_green[i]) {
        saw_red_after_green = true;
      }
    }
  }
  const bool fell =
      current.circles[0].center.y() > initial.circles[0].center.y();
  EXPECT_TRUE(fell);
  EXPECT_TRUE(saw_green);
  EXPECT_TRUE(saw_red_after_green);
  sample.End();
}

TEST(Input, PressedIsTheRisingEdgeOfHeld) {
  const std::uint8_t a = ButtonMask(Button::kA);
  const std::uint8_t b = ButtonMask(Button::kB);
  const Input input = Input::FromHeld(static_cast<std::uint8_t>(a | b), b);
  EXPECT_TRUE(input.IsHeld(Button::kA));
  EXPECT_TRUE(input.IsHeld(Button::kB));
  EXPECT_TRUE(input.WasPressed(Button::kA));
  EXPECT_FALSE(input.WasPressed(Button::kB));
  EXPECT_FALSE(input.IsHeld(Button::kUp));
}

TEST(Input, EveryButtonHasItsOwnBit) {
  constexpr Button kButtons[] = {Button::kUp,    Button::kDown, Button::kLeft,
                                 Button::kRight, Button::kA,    Button::kB,
                                 Button::kX,     Button::kY};
  unsigned seen = 0;
  for (const Button button : kButtons) {
    const std::uint8_t mask = ButtonMask(button);
    EXPECT_NE(mask, 0);
    EXPECT_EQ(seen & mask, 0U);
    seen |= mask;

    const Input input = Input::FromHeld(mask, 0);
    EXPECT_TRUE(input.IsHeld(button));
    EXPECT_TRUE(input.WasPressed(button));
  }
}

}  // namespace
}  // namespace gpr925
