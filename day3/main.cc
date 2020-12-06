#include <fstream>
#include <iostream>

#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "glog/logging.h"

constexpr int kGridSize = 20000;

struct Vector {
  char dir;  // U, D, L, R
  int magnitude;
};

typedef std::vector<std::vector<int>> Grid;

std::vector<Vector> GetPathFromFile(std::ifstream& file) {
  std::vector<Vector> path;
  std::string line;
  CHECK(std::getline(file, line));
  for (auto s : absl::StrSplit(line, ",")) {
    char dir = s[0];
    int magnitude;
    CHECK(absl::SimpleAtoi(s.substr(1), &magnitude));
    path.push_back({dir, magnitude});
  }
  return path;
}

struct Pos {
  int x;
  int y;
};

Pos Step(Pos pos, std::tuple<int, int> step) {
  auto [x_step, y_step] = step;
  return {pos.x + x_step, pos.y + y_step};
}

// Walks the given path and calls visit with x, y, and current path length.
void WalkPath(Grid& grid, std::vector<Vector> path, int origin_x, int origin_y,
              std::function<void(int, int, int)> visit) {
  int path_length = 0;

  Pos pos{origin_x, origin_y};
  std::tuple<int, int> step;
  for (auto vec : path) {
    if (vec.dir == 'U') {
      step = {0, 1};
    } else if (vec.dir == 'D') {
      step = {0, -1};
    } else if (vec.dir == 'L') {
      step = {-1, 0};
    } else {
      step = {1, 0};
    }

    // Walk the vector, visiting each element.
    for (int i = 0; i < vec.magnitude; ++i) {
      visit(pos.x, pos.y, path_length++);
      pos = Step(pos, step);
    }
  }
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  FLAGS_logtostderr = 1;

  std::ifstream file(argv[1]);
  CHECK(file);

  // Make a huge empty grid.
  Grid grid;
  for (int i = 0; i < kGridSize; ++i) {
    grid.push_back(std::vector<int>(kGridSize, -1));
  }

  int origin = kGridSize / 2;

  // Get the first path and trace it by writing the distance walked so far.
  std::vector<Vector> first_path = GetPathFromFile(file);
  int path_length = 0;
  WalkPath(grid, first_path, origin, origin,
           [&](int x, int y, int path_length) {
             int& loc = grid[y][x];
             if (loc == -1) {
               loc = path_length;
             }
           });

  // Rewrite the origin so we don't accidentally intersect with it.
  grid[origin][origin] = -1;

  // Get the second path and trace it. For part1, we'll calculate the smallest
  // manhattan distance intersection. For part2, the shortest path length.
  std::vector<Vector> second_path = GetPathFromFile(file);
  int manhattan_distance = INT_MAX;
  int walk_distance = INT_MAX;
  path_length = 0;
  WalkPath(
      grid, second_path, origin, origin, [&](int x, int y, int path_length) {
        int& loc = grid[y][x];
        if (loc != -1) {
          manhattan_distance = std::min(
              manhattan_distance, std::abs(x - origin) + std::abs(y - origin));
          walk_distance = std::min(walk_distance, loc + path_length);
        }
      });

  LOG(INFO) << "PART 1: " << manhattan_distance;
  LOG(INFO) << "PART 2: " << walk_distance;
  return 0;
}
