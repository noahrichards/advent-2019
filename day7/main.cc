#include <algorithm>
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

// A helper to run the program with the given input and return the (single)
// output.
int RunAmplifier(Memory memory, int phase_setting, int input_signal) {
  Machine machine(memory);
  machine.input() = {phase_setting, input_signal};
  CHECK(machine.Execute() == kHaltInstruction);
  CHECK(machine.output().size() == 1);
  return machine.output()[0];
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  FLAGS_logtostderr = 1;

  std::ifstream file(argv[1]);
  CHECK(file);
  Memory memory = ReadMemoryFromFile(file);

  // Part 1: we have 5 amplifiers, each of which has a unique phase setting of
  // zero through 4. For each combation of phase sequence, we feed the outputs
  // of each to the inputs of the next (input of the first is 0) and then take
  // the final output to know the maximum thrust.
  {
    // All combinations of 0, 1, 2, 3, 4.
    std::array<int, 5> phases = {0, 1, 2, 3, 4};
    int highest_output = INT_MIN;
    do {
      int last_output = 0;
      for (int i = 0; i < 5; ++i) {
        last_output = RunAmplifier(memory, phases[i], last_output);
      }
      if (last_output > highest_output) {
        highest_output = last_output;
      }
    } while (std::next_permutation(std::begin(phases), std::end(phases)));
    LOG(INFO) << "PART 1: " << highest_output;
  }

  // Part 2: run in continuous mode. Here we'll use a cooperative multitasking
  // setup; machines will halt (with kWaitingForInput) if they need but don't
  // have input, and we'll attach the output of each machine to the input of the
  // previous (wrapping around from the last amplifier to the first). We'll also
  // seed the outputs(->inputs) of the amplifiers with the phases and also the
  // last amplifier with the value "0" for the initial setting. Then we run them
  // until they all halt and check the highest value.
  {
    // All combinations of 5, 6, 7, 8, 9.
    std::array<int, 5> phases = {5, 6, 7, 8, 9};
    int highest_output = INT_MIN;
    do {
      // Make the 5 machines.
      std::array<Machine, 5> machines{Machine(memory), Machine(memory),
                                      Machine(memory), Machine(memory),
                                      Machine(memory)};
      // Hook up the outputs of each machine to the inputs of the previous.
      for (int i = 0; i < 4; ++i) {
        machines[i + 1].SetExternalInput(&machines[i].output());
      }
      machines[0].SetExternalInput(&machines[4].output());
      // Setup the machines.
      for (int i = 0; i < 5; ++i) {
        machines[i].input().push_back(phases[i]);
      }
      // Write 0 to the initial machines input.
      machines[0].input().push_back(0);

      // Now: execute the machines until they all halt.
      bool any_waiting_for_input;
      do {
        any_waiting_for_input = false;
        for (int i = 0; i < 5; ++i) {
          if (machines[i].Execute() == kWaitingForInput) {
            any_waiting_for_input = true;
          }
        }
      } while (any_waiting_for_input);
      int last_output = machines[4].output()[machines[4].output().size() - 1];
      if (last_output > highest_output) {
        highest_output = last_output;
      }
    } while (std::next_permutation(std::begin(phases), std::end(phases)));
    LOG(INFO) << "PART 2: " << highest_output;
  }

  return 0;
}
