#include <gtest/gtest.h>

#include "gpr925/scalar.h"

#include "main/samples/01_hello.h"
#include "main/samples/02_planet.h"
#include "main/samples/03_planetFourVec2.h"
#include "main/samples/sample.h"
#include "main/samples/sample_config.h"

namespace gpr925 {
namespace {

constexpr int kStepCount = 600;

bool IsInside(Vec2 point, const Rect& bounds, Scalar margin) {
  return point.x >= bounds.min_bound.x - margin &&
         point.x <= bounds.max_bound.x + margin &&
         point.y >= bounds.min_bound.y - margin &&
         point.y <= bounds.max_bound.y + margin;
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
  EXPECT_GT(sample.player_position().x, start.x);
  EXPECT_EQ(sample.player_position().y, start.y);

  const Input up = Input::FromHeld(ButtonMask(Button::kUp), 0);
  for (int step = 0; step < 30; ++step) {
    sample.FixedUpdate(up);
  }
  EXPECT_LT(sample.player_position().y, start.y);
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
        << "planet " << i << " at " << ToFloat(position.x) << ", "
        << ToFloat(position.y);
  }
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
