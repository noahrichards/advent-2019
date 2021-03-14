#ifndef INTCODE_INTCODE_H_
#define INTCODE_INTCODE_H_

#include <fstream>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_split.h"

// Storage, like tape.
typedef std::vector<int64_t> Storage;
// Memory, random access.
typedef absl::flat_hash_map<int64_t, int64_t> Memory;

// Reason the machine halted when executing.
enum HaltReason {
  // Input exhausted, call Execute() again to retry.
  kWaitingForInput,
  // Executed a Halt instruction, program is complete.
  kHaltInstruction
};

// The type of parameter, when loading/storing.
enum ParameterMode {
  // The parameter is an address in memory.
  kPosition = 0,
  // The parameter is the value itself.
  kImmediate = 1,
  // The parameter is an address in memory; the load/store should add the
  // relative_base register.
  kRelative = 2,
};

// Reads Memory from an input file.
Memory ReadMemoryFromFile(std::ifstream& file);

// Helper to run a machine with memory and a single input value and print the
// resulting output.
void RunMachine(Memory memory, int64_t input_value);

class Machine {
 public:
  explicit Machine(Memory memory) : memory_(memory), input_(&owned_input_) {}

  // Uses external input instead of the default internal input.
  void SetExternalInput(Storage* external_input);

  // Executes what is in memory. If kWaitingForInput is returned, call Execute
  // again to continue running the program when more input is available.
  HaltReason Execute();

  // Memory, modified during execution.
  Memory& memory() { return memory_; }

  // Program input.
  Storage& input() { return *input_; }

  // Program output, empty by default. Mutable so it can be used as the input
  // for another Machine.
  Storage& output() { return output_; }

 private:
  // Reads from the address at pc_ with the given mode and increments pc_.
  int64_t Read(ParameterMode parameter_mode);
  // Stores value to the address at pc_ with the given mode and increments pc_.
  void Store(int64_t value, ParameterMode mode);

  Memory memory_;
  Storage owned_input_;
  Storage output_;
  // Input storage. May be external if set via SetExternalInput. Defaults to
  // |owned_input|.
  Storage* input_;

  // Program counter, starts at zero.
  int64_t pc_ = 0;
  // Relative base register, used for relative-base parameter modes.
  int64_t relative_base_ = 0;
  // Current read location in input.
  int64_t input_loc_ = 0;
};

#endif  // INTCODE_INTCODE_H_
