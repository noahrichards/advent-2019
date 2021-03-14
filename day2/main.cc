#include <fstream>

#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "glog/logging.h"
#include "intcode/intcode.h"

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  FLAGS_logtostderr = 1;

  std::ifstream file(argv[1]);
  CHECK(file);

  Memory memory = ReadMemoryFromFile(file);

  // Part1:
  // To do this, before running the program, replace position 1 with the value
  // 12 and replace position 2 with the value 2. What value is left at position
  // 0 after the program halts?
  {
    Memory part1_memory = memory;
    part1_memory[1] = 12;
    part1_memory[2] = 2;
    Machine machine(part1_memory);
    machine.Execute();
    LOG(INFO) << "Part 1: " << machine.memory()[0];
  }

  // Part2: find values for 1 and 2 (above) that produce the output 19690720.
  // What is 100 * noun + verb?
  for (int i = 0; i < 100; ++i) {
    for (int j = 0; j < 100; ++j) {
      Memory part2_memory = memory;
      part2_memory[1] = i;
      part2_memory[2] = j;
      Machine machine(part2_memory);
      machine.Execute();
      if (machine.memory()[0] == 19690720) {
        LOG(INFO) << "Part 2: " << (100 * i) + j;
        return 0;
      }
    }
  }
  return 0;
}
