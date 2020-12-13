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

void Execute(Storage& storage, const Storage& input, Storage& output) {
  int pc = 0;
  int input_loc = 0;

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

  // Part 1: use 1 as input, print output[output.size - 1].
  {
    std::vector<int> part1_storage = storage;
    std::vector<int> input = {1};
    std::vector<int> output;
    Execute(part1_storage, input, output);
    LOG(INFO) << "PART 1 OUTPUT";
    for (int i : output) {
      LOG(INFO) << i;
    }
  }
  {
    std::vector<int> part2_storage = storage;
    std::vector<int> input = {5};
    std::vector<int> output;
    Execute(part2_storage, input, output);
    LOG(INFO) << "PART 2 OUTPUT";
    for (int i : output) {
      LOG(INFO) << i;
    }
  }
  return 0;
}
