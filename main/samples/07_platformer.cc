#include "main/samples/07_platformer.h"

#include <array>
#include <cstddef>

#include "gpr925/scalar.h"

namespace gpr925 {
namespace {

struct Platform {
  Scalar left;
  Scalar right;
  Scalar top;
};

constexpr std::array<std::array<Platform, 6>, 3> kLevels = {{
    std::array<Platform, 6>{{
        {Ratio(0, 1), Ratio(1, 1), Ratio(94, 100)},
        {Ratio(12, 100), Ratio(32, 100), Ratio(78, 100)},
        {Ratio(37, 100), Ratio(57, 100), Ratio(63, 100)},
        {Ratio(63, 100), Ratio(1, 1), Ratio(48, 100)},
        {Ratio(1, 1), Scalar{}, Scalar{}},
        {Ratio(1, 1), Scalar{}, Scalar{}},
    }},
    std::array<Platform, 6>{{
        {Ratio(0, 1), Ratio(1, 1), Ratio(94, 100)},
        {Ratio(12, 100), Ratio(29, 100), Ratio(78, 100)},
        {Ratio(34, 100), Ratio(51, 100), Ratio(64, 100)},
        {Ratio(56, 100), Ratio(73, 100), Ratio(50, 100)},
        {Ratio(78, 100), Ratio(1, 1), Ratio(36, 100)},
        {Ratio(1, 1), Scalar{}, Scalar{}},
    }},
    std::array<Platform, 6>{{
        {Ratio(0, 1), Ratio(1, 1), Ratio(94, 100)},
        {Ratio(10, 100), Ratio(25, 100), Ratio(79, 100)},
        {Ratio(29, 100), Ratio(44, 100), Ratio(66, 100)},
        {Ratio(48, 100), Ratio(63, 100), Ratio(53, 100)},
        {Ratio(67, 100), Ratio(82, 100), Ratio(40, 100)},
        {Ratio(86, 100), Ratio(1, 1), Ratio(27, 100)},
    }},
}};
constexpr std::array<std::size_t, 3> kPlatformCounts = {4, 5, 6};
constexpr Scalar kPlayerHalfWidth = Ratio(1, 5);
constexpr Scalar kPlayerHalfHeight = Ratio(7, 20);
constexpr Scalar kPlatformThickness = Ratio(1, 4);
constexpr Scalar kPlayerSpeed = Ratio(9, 2);
constexpr Scalar kJumpSpeed = Ratio(17, 2);
constexpr Scalar kGravity = ToScalar(18);
constexpr Scalar kExitHalfWidth = Ratio(1, 4);
constexpr Scalar kExitHalfHeight = Ratio(2, 5);

}  // namespace

void PlatformerSample::Begin(const SampleContext& context) {
  bounds_ = context.world_bounds;
  level_ = 0;
  grounded_ = false;
  completed_ = false;
  player_velocity_ = {};
  const Scalar floor_top =
      bounds_.min_bound.y() + bounds_.Size().y() * kLevels[level_][0].top;
  player_position_ = {
      bounds_.min_bound.x() + bounds_.Size().x() * Ratio(5, 100),
      floor_top - kPlayerHalfHeight};
}

void PlatformerSample::FixedUpdate(const Input& input) {
  if (input.WasPressed(Button::kB)) {
    Begin(SampleContext{bounds_});
    return;
  }
  if (completed_) {
    return;
  }

  const Vec2 size = bounds_.Size();
  const Scalar delta_time = kFixedDeltaTime;
  Scalar horizontal_direction{};
  if (input.IsHeld(Button::kLeft)) {
    horizontal_direction -= ToScalar(1);
  }
  if (input.IsHeld(Button::kRight)) {
    horizontal_direction += ToScalar(1);
  }
  player_velocity_.x() = horizontal_direction * kPlayerSpeed;
  const Scalar previous_x = player_position_.x();
  player_position_.x() += player_velocity_.x() * delta_time;
  player_position_.x() =
      player_position_.x() < bounds_.min_bound.x() + kPlayerHalfWidth
          ? bounds_.min_bound.x() + kPlayerHalfWidth
          : (player_position_.x() > bounds_.max_bound.x() - kPlayerHalfWidth
                 ? bounds_.max_bound.x() - kPlayerHalfWidth
                 : player_position_.x());

  for (std::size_t i = 1; i < kPlatformCounts[level_]; ++i) {
    const Platform& platform = kLevels[level_][i];
    const Scalar platform_left =
        bounds_.min_bound.x() + size.x() * platform.left;
    const Scalar platform_right =
        bounds_.min_bound.x() + size.x() * platform.right;
    const Scalar platform_top =
        bounds_.min_bound.y() + size.y() * platform.top;
    const bool overlaps_vertically =
        player_position_.y() + kPlayerHalfHeight > platform_top &&
        player_position_.y() - kPlayerHalfHeight <
            platform_top + kPlatformThickness;
    const bool overlaps_horizontally =
        player_position_.x() + kPlayerHalfWidth > platform_left &&
        player_position_.x() - kPlayerHalfWidth < platform_right;
    if (overlaps_vertically && overlaps_horizontally &&
        player_velocity_.x() > Scalar{} &&
        previous_x + kPlayerHalfWidth <= platform_left) {
      player_position_.x() = platform_left - kPlayerHalfWidth;
    } else if (overlaps_vertically && overlaps_horizontally &&
               player_velocity_.x() < Scalar{} &&
               previous_x - kPlayerHalfWidth >= platform_right) {
      player_position_.x() = platform_right + kPlayerHalfWidth;
    }
  }

  if (grounded_ &&
      (input.WasPressed(Button::kA) || input.WasPressed(Button::kUp))) {
    player_velocity_.y() = -kJumpSpeed;
    grounded_ = false;
  }

  const Scalar previous_y = player_position_.y();
  player_velocity_.y() += kGravity * delta_time;
  player_position_.y() += player_velocity_.y() * delta_time;
  grounded_ = false;
  for (std::size_t i = 0; i < kPlatformCounts[level_]; ++i) {
    const Platform& platform = kLevels[level_][i];
    const Scalar platform_left =
        bounds_.min_bound.x() + size.x() * platform.left;
    const Scalar platform_right =
        bounds_.min_bound.x() + size.x() * platform.right;
    const Scalar platform_top =
        bounds_.min_bound.y() + size.y() * platform.top;
    const Scalar platform_bottom = platform_top + kPlatformThickness;
    const bool overlaps_horizontally =
        player_position_.x() + kPlayerHalfWidth > platform_left &&
        player_position_.x() - kPlayerHalfWidth < platform_right;

    if (!overlaps_horizontally) {
      continue;
    }
    if (player_velocity_.y() >= Scalar{} &&
        previous_y + kPlayerHalfHeight <= platform_top &&
        player_position_.y() + kPlayerHalfHeight >= platform_top) {
      player_position_.y() = platform_top - kPlayerHalfHeight;
      player_velocity_.y() = Scalar{};
      grounded_ = true;
    } else if (player_velocity_.y() < Scalar{} &&
               previous_y - kPlayerHalfHeight >= platform_bottom &&
               player_position_.y() - kPlayerHalfHeight <= platform_bottom) {
      player_position_.y() = platform_bottom + kPlayerHalfHeight;
      player_velocity_.y() = Scalar{};
    }
  }

  if (player_position_.y() < bounds_.min_bound.y() + kPlayerHalfHeight) {
    player_position_.y() = bounds_.min_bound.y() + kPlayerHalfHeight;
    player_velocity_.y() = Scalar{};
  }

  const Platform& goal_platform =
      kLevels[level_][kPlatformCounts[level_] - 1];
  const Scalar goal_x =
      bounds_.min_bound.x() + size.x() * Ratio(93, 100);
  const Scalar goal_y =
      bounds_.min_bound.y() + size.y() * goal_platform.top - kExitHalfHeight;
  if (Abs(player_position_.x() - goal_x) <
          kPlayerHalfWidth + kExitHalfWidth &&
      Abs(player_position_.y() - goal_y) <
          kPlayerHalfHeight + kExitHalfHeight) {
    if (level_ + 1 < kLevels.size()) {
      ++level_;
      const Scalar floor_top =
          bounds_.min_bound.y() +
          size.y() * kLevels[level_][0].top;
      player_position_ = {
          bounds_.min_bound.x() + size.x() * Ratio(5, 100),
          floor_top - kPlayerHalfHeight};
      player_velocity_ = {};
      grounded_ = false;
    } else {
      completed_ = true;
    }
  }
}

void PlatformerSample::Draw(DrawList& draw_list) const {
  const Vec2 size = bounds_.Size();
  draw_list.boxes.push_back(
      {bounds_.Center(), size * Vec2{Ratio(1, 2), Ratio(1, 2)},
       sample_color::kSky});

  for (std::size_t i = 0; i < kPlatformCounts[level_]; ++i) {
    const Platform& platform = kLevels[level_][i];
    const Scalar left =
        bounds_.min_bound.x() + size.x() * platform.left;
    const Scalar right =
        bounds_.min_bound.x() + size.x() * platform.right;
    const Scalar top =
        bounds_.min_bound.y() + size.y() * platform.top;
    draw_list.boxes.push_back(
        {{(left + right) / ToScalar(2),
          top + kPlatformThickness / ToScalar(2)},
         {(right - left) / ToScalar(2), kPlatformThickness / ToScalar(2)},
         i == 0 ? sample_color::kDarkGrey : sample_color::kTeal});
  }

  const Platform& goal_platform =
      kLevels[level_][kPlatformCounts[level_] - 1];
  const Scalar goal_y =
      bounds_.min_bound.y() + size.y() * goal_platform.top -
      kExitHalfHeight;
  draw_list.boxes.push_back(
      {{bounds_.min_bound.x() + size.x() * Ratio(93, 100), goal_y},
       {kExitHalfWidth, kExitHalfHeight},
       completed_ ? sample_color::kGreen : sample_color::kYellow});
  draw_list.boxes.push_back(
      {player_position_, {kPlayerHalfWidth, kPlayerHalfHeight},
       sample_color::kWhite});

  for (std::size_t i = 0; i < kLevels.size(); ++i) {
    const Scalar indicator_x =
        bounds_.min_bound.x() + Ratio(2, 5) +
        ToScalar(static_cast<int>(i)) * Ratio(3, 5);
    draw_list.boxes.push_back(
        {{indicator_x, bounds_.min_bound.y() + Ratio(2, 5)},
         {Ratio(1, 10), Ratio(1, 10)},
         i == level_ || (completed_ && i == kLevels.size() - 1)
             ? sample_color::kGreen
             : sample_color::kGrey});
  }
}

void PlatformerSample::End() {
  bounds_ = {};
  player_position_ = {};
  player_velocity_ = {};
  level_ = 0;
  grounded_ = false;
  completed_ = false;
}

}  // namespace gpr925
