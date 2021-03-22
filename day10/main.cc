#include <cmath>
#include <execution>
#include <fstream>
#include <numeric>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"
#include "absl/strings/substitute.h"
#include "absl/types/optional.h"
#include "glog/logging.h"

#define PI 3.14159265

using namespace std::placeholders;

typedef std::tuple<int, int> PosT;
typedef absl::flat_hash_set<PosT> MapT;
typedef absl::flat_hash_map<PosT, absl::flat_hash_set<PosT>> EdgeMapT;

PosT operator+(PosT lhs, PosT rhs) {
  return {std::get<0>(lhs) + std::get<0>(rhs),
          std::get<1>(lhs) + std::get<1>(rhs)};
}

std::string PosString(const PosT& pos) {
  return absl::StrFormat("[%d,%d]", std::get<1>(pos), std::get<0>(pos));
}

bool AsteroidBetween(const MapT& map, PosT from, PosT to) {
  // Figure out the step, which is basically the reduced fraction.
  auto [from_row, from_col] = from;
  auto [to_row, to_col] = to;
  int delta_row = to_row - from_row;
  int delta_col = to_col - from_col;
  std::tuple<int, int> step;
  if (delta_row == 0) {
    step = {0, delta_col < 0 ? -1 : 1};
  } else if (delta_col == 0) {
    step = {delta_row < 0 ? -1 : 1, 0};
  } else {
    // Find the reduced fraction. If the GCD is 1, then there's no occlusion
    // possible.
    int gcd = std::gcd(delta_row, delta_col);
    if (gcd == 1) return false;
    step = {delta_row / gcd, delta_col / gcd};
  }
  VLOG(2) << "Step is " << PosString(step) << ", checking from "
          << PosString(from) << " to " << PosString(to);

  for (PosT pos = from + step; pos != to; pos = pos + step) {
    VLOG(2) << "CHECKING " << PosString(pos);
    if (map.count(pos) == 1) return true;
  }
  return false;
}

// Finds all asteroids on the map that have edges (are visible to each other).
EdgeMapT FindEdges(const MapT& map) {
  EdgeMapT edges;
  for (auto iter = map.begin(); iter != map.end();) {
    auto from = *iter;
    VLOG(2) << "Checking " << PosString(from);
    for (auto other = ++iter; other != map.end(); ++other) {
      auto to = *other;
      // Walk between the two and see if anything occludes.
      if (!AsteroidBetween(map, from, to)) {
        VLOG(2) << "EDGE FOUND: " << PosString(from) << " to " << PosString(to);
        edges[from].insert(to);
        edges[to].insert(from);
      }
    }
  }
  return edges;
}

double Angle(PosT origin, PosT pos) {
  // We're upside down (0,0 is top-left, inf,inf is bottom-right) from what the
  // puzzle expects, so the row/y delta is reversed. Also PosT are row,col and
  //  not x,y, so a bit of weirdness here.
  int x_offset = std::get<1>(pos) - std::get<1>(origin);
  int y_offset = std::get<0>(origin) - std::get<0>(pos);

  double degrees = atan2(y_offset, x_offset) * 180 / PI;
  // atan2 is 0 to 180 in the counterclockwise direction and -0 to -180 is the
  // clockwise direction. We want the clockwise angle facing up, so:
  return fmod(
      // Clockwise, instead of counter clockwise.
      -degrees
          // Origin (0) up instead of right
          + 90.0
          // -90 to -0 we want to appear at the end, so adding 360 pre-fmod will
          // move everything else before.
          + 360.0
          // In practice, directly up is turning into -0, so add a tiny amount
          // to move it to the other side of the range.
          + 0.0001,
      360.0);
}

int Distance(PosT origin, PosT pos) {
  int x_offset = std::get<0>(pos) - std::get<0>(origin);
  int y_offset = std::get<1>(pos) - std::get<1>(origin);
  return std::sqrt(x_offset * x_offset + y_offset * y_offset);
}

bool CompareAsteroid(PosT origin, PosT lhs, PosT rhs) {
  if (lhs == rhs) return false;
  auto lhs_angle = Angle(origin, lhs);
  auto rhs_angle = Angle(origin, rhs);
  if (lhs_angle != rhs_angle) return lhs_angle < rhs_angle;
  // If the angles are the same, the nearer one should order first.
  return Distance(origin, lhs) < Distance(origin, rhs);
}

int main(int argc, char** argv) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = 1;

  std::ifstream file(argv[1]);
  CHECK(file);

  std::vector<PosT> asteroids;
  MapT map;
  int row = 0;
  std::string line;
  while (std::getline(file, line)) {
    for (int col = 0; col < line.size(); ++col) {
      if (line[col] == '#') {
        map.insert({row, col});
        asteroids.push_back({row, col});
      }
    }
    ++row;
  }

  auto edges = FindEdges(map);

  // Part1: find the most asteroids detected.
  size_t max = 0;
  PosT best_pos;
  for (auto [pos, others] : edges) {
    if (others.size() > max) {
      max = others.size();
      best_pos = pos;
    }
  }
  LOG(INFO) << "BEST: " << PosString(best_pos);

  LOG(INFO) << "PART 1: " << max;

  // Part2: find the 200th asteroid vaporized.

  // Into buckets by the angle to the best pos.
  absl::flat_hash_map<double, std::list<PosT>> by_angle;
  std::set<double> angles;
  for (const auto& pos : asteroids) {
    if (pos == best_pos) continue;
    double angle = Angle(best_pos, pos);
    by_angle[angle].push_back(pos);
    angles.insert(angle);
  }
  // Sort each bucket so the nearest asteroid appears first.
  for (auto& [angle, bucket] : by_angle) {
    bucket.sort([&](const auto& lhs, const auto& rhs) {
      return Distance(best_pos, lhs) < Distance(best_pos, rhs);
    });
  }

  // Now enumerate the buckets and clear out the lowest in each bucket as we go.
  int count = 1;
  PosT asteroid_200;
  bool all_empty;
  do {
    all_empty = true;
    for (auto angle : angles) {
      auto iter = by_angle.find(angle);
      if (iter == by_angle.end()) continue;
      all_empty = false;
      auto pos = iter->second.front();
      iter->second.pop_front();

      VLOG(2) << count << ": " << PosString(pos) << " (" << Angle(best_pos, pos)
              << ")";
      if (count == 200) {
        asteroid_200 = pos;
      }
      if (iter->second.empty()) {
        by_angle.erase(iter);
      }
      ++count;
    }
  } while (!all_empty);
  LOG(INFO) << "PART 2: "
            << std::get<1>(asteroid_200) * 100 + std::get<0>(asteroid_200)
            << "   " << PosString(asteroid_200);
  return 0;
}
