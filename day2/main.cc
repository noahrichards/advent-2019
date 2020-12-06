#include <fstream>

#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "glog/logging.h"

constexpr int kAdd = 1;
constexpr int kMult = 2;
constexpr int kStop = 99;

void Execute(std::vector<int>& storage) {
  int pc = 0;

  while (storage[pc] != kStop) {
    if (storage[pc] == kAdd) {
      storage[storage[pc + 3]] =
          storage[storage[pc + 1]] + storage[storage[pc + 2]];
      pc += 4;
    } else if (storage[pc] == kMult) {
      storage[storage[pc + 3]] =
          storage[storage[pc + 1]] * storage[storage[pc + 2]];
      pc += 4;
    } else {
      CHECK(false);
    }
  }
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  FLAGS_logtostderr = 1;

  std::ifstream file(argv[1]);
  CHECK(file);

  std::vector<int> storage;

  std::string line;
  while (std::getline(file, line)) {
    for (auto s : absl::StrSplit(line, ",")) {
      int val;
      CHECK(absl::SimpleAtoi(s, &val));
      storage.push_back(val);
    }
  }

  // Part1:
  // To do this, before running the program, replace position 1 with the value
  // 12 and replace position 2 with the value 2. What value is left at position
  // 0 after the program halts?
  auto part1_storage = storage;
  part1_storage[1] = 12;
  part1_storage[2] = 2;
  Execute(part1_storage);
  LOG(INFO) << "Part 1: " << part1_storage[0];

  // Part2: find values for 1 and 2 (above) that produce the output 19690720.
  // What is 100 * noun + verb?
  for (int i = 0; i < 100; ++i) {
    for (int j = 0; j < 100; ++j) {
      auto part2_storage = storage;
      part2_storage[1] = i;
      part2_storage[2] = j;
      Execute(part2_storage);
      if (part2_storage[0] == 19690720) {
        LOG(INFO) << "Part 2: " << (100 * i) + j;
        return 0;
      }
    }
  }
  return 0;
}
