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

constexpr int kAdd = 1;
constexpr int kMult = 2;
constexpr int kStore = 3;
constexpr int kOutput = 4;
constexpr int kJumpIfTrue = 5;
constexpr int kJumpIfFalse = 6;
constexpr int kLessThan = 7;
constexpr int kEquals = 8;
constexpr int kHalt = 99;

std::string OpName(int op) {
  switch (op) {
    case kAdd:
      return "add";
    case kMult:
      return "mult";
    case kStore:
      return "store";
    case kOutput:
      return "output";
    case kJumpIfTrue:
      return "jtrue";
    case kJumpIfFalse:
      return "jfalse";
    case kLessThan:
      return "jlt";
    case kEquals:
      return "jeq";
    case kHalt:
      return "halt";
    default:
      CHECK(false) << "Unknown op: " << op;
  }
}

constexpr int kPosition = 0;
constexpr int kImmediate = 1;

typedef std::vector<int> Storage;

struct Instruction {
  int opcode;
  std::array<int, 3> modes = {0, 0, 0};
};

Instruction ParseInstruction(int value) {
  int original_for_log = value;
  Instruction instruction;
  instruction.opcode = value % 100;
  value /= 100;
  int i = 0;
  while (value > 0) {
    instruction.modes[i++] = value % 10;
    CHECK(instruction.modes[i - 1] < 2) << "full value: " << original_for_log;
    value /= 10;
  }
  return instruction;
}

int Read(const Storage& storage, int value, int mode) {
  switch (mode) {
    case kPosition:
      VLOG(2) << "Value is (pos): " << storage[value];
      return storage[value];
    case kImmediate:
      VLOG(2) << "Value is (imm): " << value;
      return value;
    default:
      CHECK(false) << "Unknown mode: " << mode;
  }
}

void Store(Storage& storage, int address, int value) {
  VLOG(2) << "Storing " << value << " to " << address;
  storage[address] = value;
}

bool MatchesBoolean(int value, bool b) { return b ? value > 0 : value == 0; }

// A machine state. Execute will execute the machine from whatever state it is
// in.
//
// A machine owns its output but not its input.
struct Machine {
  Storage storage;
  Storage* input;
  Storage output;
  int pc = 0;
  int input_loc = 0;
};

enum HaltReason { kWaitingForInput, KHaltInstruction };

HaltReason Execute(Machine& machine) {
  CHECK(machine.input);

  int& pc = machine.pc;
  int& input_loc = machine.input_loc;
  Storage& storage = machine.storage;
  Storage& input = *machine.input;
  Storage& output = machine.output;

  while (storage[pc] != kHalt) {
    Instruction i = ParseInstruction(storage[pc++]);
    VLOG(1) << "[" << pc - 1 << "] Executing op: " << OpName(i.opcode);
    switch (i.opcode) {
      case kAdd: {
        int val1 = Read(storage, storage[pc++], i.modes[0]);
        int val2 = Read(storage, storage[pc++], i.modes[1]);
        Store(storage, storage[pc++], val1 + val2);
        break;
      }
      case kMult: {
        int val1 = Read(storage, storage[pc++], i.modes[0]);
        int val2 = Read(storage, storage[pc++], i.modes[1]);
        Store(storage, storage[pc++], val1 * val2);
        break;
      }
      case kStore: {
        // If we don't have enough input yet, restore the pc and return.
        if (input_loc >= input.size()) {
          --pc;
          return kWaitingForInput;
        }
        int val = input[input_loc++];
        VLOG(2) << "Read " << val << " from input.";
        Store(storage, storage[pc++], val);
        break;
      }
      case kOutput: {
        int val = Read(storage, storage[pc++], i.modes[0]);
        output.push_back(val);
        break;
      }
      case kJumpIfTrue:
      case kJumpIfFalse: {
        int val = Read(storage, storage[pc++], i.modes[0]);
        int jump_to = Read(storage, storage[pc++], i.modes[1]);
        if (MatchesBoolean(val, i.opcode == kJumpIfTrue)) {
          VLOG(2) << "Jumping to " << jump_to;
          pc = jump_to;
        } else {
          VLOG(2) << "No jump.";
        }
        break;
      }
      case kLessThan:
      case kEquals: {
        int val1 = Read(storage, storage[pc++], i.modes[0]);
        int val2 = Read(storage, storage[pc++], i.modes[1]);
        int result;
        if (i.opcode == kEquals) {
          result = val1 == val2 ? 1 : 0;
        } else {
          result = val1 < val2 ? 1 : 0;
        }
        Store(storage, storage[pc++], result);
        break;
      }
      default:
        CHECK(false) << "Unknown opcode: " << storage[pc];
    }
  }
  return kHaltInstruction;
}

// A helper to run the program with the given input and return the (single)
// output.
int RunAmplifier(std::vector<int> storage, int phase_setting,
                 int input_signal) {
  std::vector<int> input = {phase_setting, input_signal};
  Machine machine = {storage, &input};
  CHECK(Execute({storage, &input}) == kHaltInstruction);
  CHECK(machine.output.size() == 1);
  return machine.output[0];
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
        last_output = RunAmplifier(storage, phases[i], last_output);
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
      std::array<Machine, 5> machines;
      // Hook up the outputs of each machine to the inputs of the previous.
      for (int i = 0; i < 4; ++i) machines[i + 1].input = &machines[i].output;
      machines[0].input = &machines[4].output;
      // Write the phase settings to the inputs.
      for (int i = 0; i < 5; ++i) machines[i].input->push_back(phases[i]);
      // Write 0 to the initial machines input.
      machines[0].input->push_back(0);

      // Now: execute the machines until they all halt.
      do {
        bool any_waiting_for_input = false;
        for (int i = 0; i < 5; ++i) {
          if (Execute(machines[i]) == kWaitingForInput) {
            any_waiting_for_input = true;
          }
        }
      } while (any_waiting_for_input);
      int last_output = machines[4].output[machines[4].output.size() - 1];
      if (last_output > highest_output) {
        highest_output = last_output;
      }
    } while (std::next_permutation(std::begin(phases), std::end(phases)));
    LOG(INFO) << "PART 2: " << highest_output;
  }

  return 0;
}
