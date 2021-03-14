#include "intcode/intcode.h"

#include "glog/logging.h"

namespace {

enum OpCode {
  kAdd = 1,
  kMult = 2,
  kStore = 3,
  kOutput = 4,
  kJumpIfTrue = 5,
  kJumpIfFalse = 6,
  kLessThan = 7,
  kEquals = 8,
  kAdjustRelativeBase = 9,
  kHalt = 99,
};

class Instruction {
 public:
  explicit Instruction(int64_t value) {
    // If this is an instruction, it should fit in an int and be positive.
    CHECK_GT(value, 0);
    CHECK_LT(value, INT_MAX);
    auto original_for_log = value;
    op_code_ = static_cast<OpCode>(value % 100);
    value /= 100;
    int mode_index = 0;
    while (value > 0) {
      ParameterMode mode = static_cast<ParameterMode>(value % 10);
      CHECK_GT(mode, -1) << "Invalid mode parse for " << original_for_log;
      CHECK_LT(mode, 4) << "Invalid mode parse for " << original_for_log;
      modes_[mode_index++] = mode;
      value /= 10;
    }
  }

  OpCode op() const { return op_code_; }
  ParameterMode mode(int index) { return modes_[index]; }

 private:
  OpCode op_code_;
  std::array<ParameterMode, 3> modes_ = {kPosition, kPosition, kPosition};
};

// For debugging
std::string OpName(OpCode op) {
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
    case kAdjustRelativeBase:
      return "rbadj";
    case kHalt:
      return "halt";
    default:
      CHECK(false) << "Unknown op: " << op;
  }
}

bool MatchesBoolean(int64_t value, bool b) {
  return b ? value > 0 : value == 0;
}

}  // namespace

Memory ReadMemoryFromFile(std::ifstream& file) {
  Memory memory;
  int64_t cursor = 0;
  std::string line;
  while (std::getline(file, line)) {
    for (auto s : absl::StrSplit(line, ",")) {
      int64_t val;
      CHECK(absl::SimpleAtoi(s, &val));
      memory.insert({cursor++, val});
    }
  }
  return memory;
}

void RunMachine(Memory memory, int64_t input_value) {
  Machine machine(memory);
  machine.input() = {input_value};
  machine.Execute();
  for (auto i : machine.output()) {
    LOG(INFO) << i;
  }
}

void Machine::SetExternalInput(Storage* external_input) {
  CHECK(external_input);
  input_ = external_input;
}

int64_t Machine::Read(ParameterMode mode) {
  int64_t parameter = memory_[pc_++];
  switch (mode) {
    case kPosition:
      return memory_[parameter];
    case kImmediate:
      return parameter;
    case kRelative:
      return memory_[parameter + relative_base_];
    default:
      CHECK(false) << "Unknown mode: " << mode;
  }
}

void Machine::Store(int64_t value, ParameterMode mode) {
  int64_t address = memory_[pc_++];
  switch (mode) {
    case kPosition:
      memory_[address] = value;
      break;
    case kImmediate:
      CHECK(false) << "Writes will never use immediate mode.";
      break;
    case kRelative:
      memory_[address + relative_base_] = value;
      break;
    default:
      CHECK(false) << "Unknown mode: " << mode;
  }
}

HaltReason Machine::Execute() {
  while (memory_[pc_] != kHalt) {
    Instruction i(Read(kImmediate));
    VLOG(1) << "[" << pc_ - 1 << "] Executing op: " << OpName(i.op());
    switch (i.op()) {
      case kAdd: {
        auto val1 = Read(i.mode(0));
        auto val2 = Read(i.mode(1));
        Store(val1 + val2, i.mode(2));
        break;
      }
      case kMult: {
        auto val1 = Read(i.mode(0));
        auto val2 = Read(i.mode(1));
        Store(val1 * val2, i.mode(2));
        break;
      }
      case kStore: {
        // If we don't have enough input yet, restore the pc and return.
        if (input_loc_ >= input_->size()) {
          --pc_;
          return kWaitingForInput;
        }
        auto val = (*input_)[input_loc_++];
        VLOG(2) << "Read " << val << " from input.";
        Store(val, i.mode(0));
        break;
      }
      case kOutput: {
        auto val = Read(i.mode(0));
        output_.push_back(val);
        break;
      }
      case kJumpIfTrue:
      case kJumpIfFalse: {
        auto val = Read(i.mode(0));
        auto jump_to = Read(i.mode(1));
        if (MatchesBoolean(val, i.op() == kJumpIfTrue)) {
          VLOG(2) << "Jumping to " << jump_to;
          pc_ = jump_to;
        } else {
          VLOG(2) << "No jump.";
        }
        break;
      }
      case kLessThan:
      case kEquals: {
        auto val1 = Read(i.mode(0));
        auto val2 = Read(i.mode(1));
        decltype(val1) result;
        if (i.op() == kEquals) {
          result = val1 == val2 ? 1 : 0;
        } else {
          result = val1 < val2 ? 1 : 0;
        }
        Store(result, i.mode(2));
        break;
      }
      case kAdjustRelativeBase: {
        relative_base_ += Read(i.mode(0));
        VLOG(2) << "RB is: " << relative_base_;
        break;
      }
      default:
        CHECK(false) << "Unknown opcode: " << i.op();
    }
  }
  return kHaltInstruction;
}
