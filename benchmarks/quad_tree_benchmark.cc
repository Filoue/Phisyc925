#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "gpr925/quad_tree.h"

namespace gpr925 {
namespace {

using Clock = std::chrono::steady_clock;
using PointKey = std::pair<int, int>;

constexpr int kWorldSize = 10000;
constexpr int kQuerySize = 800;
constexpr std::size_t kTreeCapacity = 8;
constexpr std::size_t kQueryCount = 64;
constexpr std::size_t kBuildRepeats = 3;
constexpr std::size_t kQueryRepeats = 4;
constexpr std::size_t kSamples = 7;
constexpr std::array<std::size_t, 4> kObjectCounts{64, 256, 1024, 4096};

std::vector<PhysicsBody> MakePoints(std::size_t count) {
  std::mt19937 rng{0x925A11u};
  std::uniform_int_distribution<int> coordinate{0, kWorldSize - 1};
  std::vector<PhysicsBody> points;
  points.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    PhysicsBody point;
    point.position = {ToScalar(coordinate(rng)), ToScalar(coordinate(rng))};
    points.push_back(point);
  }
  return points;
}

std::vector<Rectangle> MakeQueries() {
  std::mt19937 rng{0x925B22u};
  std::uniform_int_distribution<int> coordinate{0, kWorldSize - kQuerySize};
  std::vector<Rectangle> queries;
  queries.reserve(kQueryCount);
  for (std::size_t i = 0; i < kQueryCount; ++i) {
    queries.emplace_back(
        Vec2{ToScalar(coordinate(rng)), ToScalar(coordinate(rng))},
        kQuerySize, kQuerySize);
  }
  return queries;
}

std::vector<PointKey> SortedKeys(const std::vector<PhysicsBody>& points) {
  std::vector<PointKey> keys;
  keys.reserve(points.size());
  for (const PhysicsBody& point : points) {
    keys.emplace_back(ToInt(point.position.x()), ToInt(point.position.y()));
  }
  std::sort(keys.begin(), keys.end());
  return keys;
}

std::vector<PhysicsBody> QueryNaive(std::vector<PhysicsBody>& points,
                                    Rectangle& range,
                                    std::vector<PhysicsBody>& found) {
  for (PhysicsBody& point : points) {
    if (range.contains(point)) {
      found.push_back(point);
    }
  }
  return found;
}

void Rebuild(QuadTree& tree, std::vector<PhysicsBody>& points) {
  tree.clear();
  for (PhysicsBody& point : points) {
    tree.insert(point);
  }
}

bool ResultsMatch(QuadTree& tree, std::vector<PhysicsBody>& points,
                  std::vector<Rectangle>& queries) {
  for (Rectangle& query : queries) {
    std::vector<PhysicsBody> tree_found;
    std::vector<PhysicsBody> naive_found;
    const std::vector<PhysicsBody> tree_result =
        tree.query(query, tree_found);
    const std::vector<PhysicsBody> naive_result =
        QueryNaive(points, query, naive_found);
    if (SortedKeys(tree_result) != SortedKeys(naive_result)) {
      return false;
    }
  }
  return true;
}

template <typename Function>
double MedianMicroseconds(Function&& function, std::size_t repeats) {
  std::vector<double> samples;
  samples.reserve(kSamples);
  for (std::size_t sample = 0; sample < kSamples; ++sample) {
    const auto start = Clock::now();
    for (std::size_t repeat = 0; repeat < repeats; ++repeat) {
      function();
    }
    const auto end = Clock::now();
    const auto elapsed =
        std::chrono::duration<double, std::micro>(end - start).count();
    samples.push_back(elapsed / static_cast<double>(repeats));
  }
  std::sort(samples.begin(), samples.end());
  return samples[samples.size() / 2];
}

}  // namespace
}  // namespace gpr925

int main() {
  using namespace gpr925;

  std::vector<Rectangle> queries = MakeQueries();
  std::cout << "objects,rebuild_median_ms,quadtree_query_us,naive_query_us,"
               "query_speedup\n";

  for (const std::size_t object_count : kObjectCounts) {
    std::vector<PhysicsBody> points = MakePoints(object_count);
    Rectangle boundary{Vec2{ToScalar(0), ToScalar(0)}, kWorldSize,
                       kWorldSize};
    QuadTree tree{boundary, kTreeCapacity};

    Rebuild(tree, points);
    if (!ResultsMatch(tree, points, queries)) {
      std::cerr << "QuadTree results differ from the linear scan at "
                << object_count << " objects\n";
      return 1;
    }

    const double rebuild_microseconds =
        MedianMicroseconds([&] { Rebuild(tree, points); }, kBuildRepeats);

    std::size_t result_count = 0;
    const auto measure_queries = [&](bool use_tree) {
      for (Rectangle& query : queries) {
        if (use_tree) {
          std::vector<PhysicsBody> found;
          const std::vector<PhysicsBody> result = tree.query(query, found);
          result_count += result.size();
        } else {
          std::vector<PhysicsBody> found;
          const std::vector<PhysicsBody> result =
              QueryNaive(points, query, found);
          result_count += result.size();
        }
      }
    };

    const double tree_query_microseconds =
        MedianMicroseconds([&] { measure_queries(true); }, kQueryRepeats) /
        static_cast<double>(kQueryCount);
    const double naive_query_microseconds =
        MedianMicroseconds([&] { measure_queries(false); }, kQueryRepeats) /
        static_cast<double>(kQueryCount);
    const double speedup = naive_query_microseconds / tree_query_microseconds;

    std::cout << object_count << ',' << std::fixed << std::setprecision(3)
              << rebuild_microseconds / 1000.0 << ','
              << tree_query_microseconds << ',' << naive_query_microseconds
              << ',' << std::setprecision(2) << speedup << '\n';
    if (result_count == 0) {
      std::cerr << "Benchmark did not consume any query results\n";
      return 1;
    }
  }
  return 0;
}
