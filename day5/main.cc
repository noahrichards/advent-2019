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
#include "intcode/intcode.h"

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  FLAGS_logtostderr = 1;

  std::ifstream file(argv[1]);
  CHECK(file);

  Memory memory = ReadMemoryFromFile(file);

  // Part 1: use 1 as input, print output[output.size - 1].
  LOG(INFO) << "PART 1 OUTPUT";
  RunMachine(memory, 1);

  LOG(INFO) << "PART 2 OUTPUT";
  RunMachine(memory, 5);
  return 0;
}
