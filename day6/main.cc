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

typedef absl::flat_hash_map<std::string, absl::flat_hash_set<std::string>>
    Orbits;

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  FLAGS_logtostderr = 1;

  std::ifstream file(argv[1]);
  CHECK(file);

  Orbits orbits;
  absl::flat_hash_map<std::string, std::string> reverse_orbits;
  std::string line;
  while (std::getline(file, line)) {
    std::vector<std::string> objects = absl::StrSplit(line, ")");
    CHECK_EQ(objects.size(), 2);
    // Add the orbit.
    {
      auto [orbit_iter, _] = orbits.try_emplace(objects[0]);
      orbit_iter->second.insert(objects[1]);
    }
    // The reverse orbit is always 1:1.
    {
      auto [_, inserted] = reverse_orbits.try_emplace(objects[1], objects[0]);
      CHECK(inserted) << "Already known: " << objects[1];
    }
  }

  // Part 1: use the reverse-map to find the length of paths.
  int total = 0;
  for (auto [key, _] : reverse_orbits) {
    auto iter = reverse_orbits.find(key);
    while (iter != reverse_orbits.end()) {
      ++total;
      iter = reverse_orbits.find(iter->second);
    }
  }
  LOG(INFO) << "PART 1: " << total;

  // Part 2: go from YOU to the object that SAN is orbiting. Store YOUR parent
  // chain with distance to each (where the path to the immediate parent for
  // each is length zero) and then walk SAN's parents until you find the first
  // place they overlap.
  {
    absl::flat_hash_map<std::string, int> my_parent_chain;
    auto iter = reverse_orbits.find("YOU");
    int distance = 0;
    while (iter != reverse_orbits.end()) {
      my_parent_chain.insert({iter->second, distance++});
      iter = reverse_orbits.find(iter->second);
    }
    // Ditto, with SAN's parents, and then add the distances.
    iter = reverse_orbits.find("SAN");
    distance = 0;
    while (iter != reverse_orbits.end()) {
      auto my_iter = my_parent_chain.find(iter->second);
      if (my_iter != my_parent_chain.end()) {
        LOG(INFO) << "PART 2: " << distance + my_iter->second;
        break;
      }
      ++distance;
      iter = reverse_orbits.find(iter->second);
    }
  }

  return 0;
}
