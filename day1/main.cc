#include <fstream>

#include "absl/strings/numbers.h"
#include "glog/logging.h"

int CalculateFuel(int mass) { return mass / 3 - 2; }

int FuelForFuel(int fuel) {
  int next_fuel = CalculateFuel(fuel);
  while (next_fuel > 0) {
    fuel += next_fuel;
    next_fuel = CalculateFuel(next_fuel);
  }
  return fuel;
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  FLAGS_logtostderr = 1;

  std::ifstream file(argv[1]);
  CHECK(file);

  long part1_fuel = 0;
  long part2_fuel = 0;
  std::string line;
  while (std::getline(file, line)) {
    int mass;
    CHECK(absl::SimpleAtoi(line, &mass));
    long fuel = CalculateFuel(mass);
    part1_fuel += fuel;
    part2_fuel += FuelForFuel(fuel);
  }

  LOG(INFO) << "Part 1: " << part1_fuel;
  LOG(INFO) << "Part 2: " << part2_fuel;
  return 0;
}
