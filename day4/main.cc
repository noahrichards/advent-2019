#include <fstream>
#include <iostream>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"
#include "absl/strings/substitute.h"
#include "absl/types/optional.h"
#include "glog/logging.h"

bool IsValidPart1(int pwd) {
  bool adjacent_same = false;
  int last = INT_MAX;
  while (pwd > 0) {
    int tens = pwd % 10;
    if (tens == last) {
      adjacent_same = true;
    }
    if (tens > last) return false;
    last = tens;
    pwd /= 10;
  }
  return adjacent_same;
}

bool IsValidPart2(int pwd) {
  int copy_for_log = pwd;
  // Part1 ensures there's at least one repeated digit and they are in order.
  if (!IsValidPart1(pwd)) return false;

  std::vector<int> digits;
  while (pwd > 0) {
    digits.push_back(pwd % 10);
    pwd /= 10;
  }

  // See if there's a run of exactly two.
  for (int i = 0; i < digits.size() - 1; ++i) {
    // If there's a run from this digit but not longer than 2, this counts.
    if (digits[i] == digits[i + 1]) {
      int repeat = digits[i];
      // Eat the rest of the repeated digits (if there are any).
      int j = i + 2;
      for (; j < digits.size() && digits[j] == repeat; ++j) {
      }
      // If we didn't walk any further, this is run of two.
      if (j == i + 2) {
        return true;
      }
      // We ate more digits, so jump to the end of the sequence.
      i = j - 1;
    }
  }
  return false;
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  FLAGS_logtostderr = 1;

  std::ifstream file(argv[1]);
  CHECK(file);

  std::string line;
  CHECK(std::getline(file, line));
  std::vector<std::string> parts = absl::StrSplit(line, "-");
  int min, max;
  CHECK(absl::SimpleAtoi(parts[0], &min));
  CHECK(absl::SimpleAtoi(parts[1], &max));

  int part1 = 0;
  int part2 = 0;
  for (int i = min; i <= max; ++i) {
    if (IsValidPart1(i)) ++part1;
    if (IsValidPart2(i)) ++part2;
  }

  LOG(INFO) << "PART 1: " << part1;
  LOG(INFO) << "PART 2: " << part2;
  return 0;
}
