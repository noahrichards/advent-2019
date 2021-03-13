#include <execution>
#include <fstream>
#include <numeric>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"
#include "absl/strings/substitute.h"
#include "absl/types/optional.h"
#include "glog/logging.h"

typedef absl::flat_hash_map<char, int> HistogramT;

constexpr int kWidth = 25;
constexpr int kHeight = 6;
constexpr int kLayerSize = kWidth * kHeight;

HistogramT GetHistogram(std::string layer) {
  HistogramT histogram;
  for (char c : layer) {
    histogram[c - '0'] += 1;
  }
  return histogram;
}

std::string Collapse(std::list<std::string> layers) {
  std::string image(kLayerSize, '2');
  for (const auto& layer : layers) {
    for (int i = 0; i < kLayerSize; ++i) {
      if (image[i] == '2' && layer[i] != '2') {
        image[i] = layer[i] == '0' ? ' ' : '*';
      }
    }
  }
  return image;
}

void PrintImage(std::string image) {
  LOG(INFO) << "IMAGE:";
  for (int row = 0; row < kHeight; ++row) {
    LOG(INFO) << image.substr(row * kWidth, kWidth);
  }
}

int main(int argc, char** argv) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = 1;

  std::ifstream file(argv[1]);
  CHECK(file);

  std::string line;
  CHECK(std::getline(file, line));
  int layer_count = line.size() / kLayerSize;
  CHECK_EQ(kLayerSize * layer_count, line.size());

  char* cursor = &line[0];

  std::list<std::string> layers;
  for (int i = 0; i < layer_count; ++i) {
    layers.push_back(line.substr(i * kLayerSize, kLayerSize));
  }

  // Part 1: find the row with the fewest 0 digits.
  absl::optional<HistogramT> lowest_zero_histogram;
  for (const auto& layer : layers) {
    auto histogram = GetHistogram(layer);
    if (!lowest_zero_histogram || histogram[0] < (*lowest_zero_histogram)[0]) {
      lowest_zero_histogram = histogram;
    }
  }
  CHECK(lowest_zero_histogram);
  LOG(INFO) << "PART 1: "
            << (*lowest_zero_histogram)[1] * (*lowest_zero_histogram)[2];

  // Part 2: collapse into a single image.
  std::string image = Collapse(layers);
  PrintImage(image);
  LOG(INFO) << "PART 2: ";
  return 0;
}
