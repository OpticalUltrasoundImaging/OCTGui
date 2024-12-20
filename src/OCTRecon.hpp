#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <vector>

// NOLINTBEGIN(*-pointer-arithmetic, *-magic-numbers)

namespace OCT {

namespace fs = std::filesystem;

template <typename T>
concept Floating = std::is_same_v<T, double> || std::is_same_v<T, float>;

template <Floating T> std::vector<T> hamming(int n) {
  std::vector<T> win(n);
  for (int i = 0; i < n; ++i) {
    win[i] = 0.54 - 0.46 * std::cos(2 * 3.1415926535897932384626 * i / n);
  }
  return win;
}

template <typename T>
inline void readTextFileToArray(const fs::path &filename, std::span<T> dst) {
  std::ifstream ifs(filename);
  if (ifs.is_open()) {
    T val{};
    size_t i{};

    while (ifs >> val && i < dst.size()) {
      dst[i++] = val;
    }

    if (ifs.eof()) {
      std::cout << "Reached end of file " << filename << '\n';
    } else if (ifs.fail()) {
      std::cerr << "Failed to read value in file " << filename << '\n';
    } else if (ifs.bad()) {
      std::cerr << "Critical I/O error occured in file " << filename << '\n';
    }
  }
}

template <Floating T> struct phaseCalibUnit {
  size_t idx;
  T l_coeff;
  T r_coeff;

  friend std::istream &operator>>(std::istream &is,
                                  phaseCalibUnit<T> &calibUnit) {
    return is >> calibUnit.idx >> calibUnit.l_coeff >> calibUnit.r_coeff;
  }
};

template <Floating T> struct Calibration {
  std::vector<T> background;
  std::vector<phaseCalibUnit<T>> phaseCalib;

  Calibration(int n_samples, const fs::path &backgroundFile,
              const fs::path &phaseFile)
      : background(n_samples), phaseCalib(n_samples) {
    readTextFileToArray<T>(backgroundFile, background);
    readTextFileToArray<phaseCalibUnit<T>>(phaseFile, phaseCalib);
  }
};

} // namespace OCT

// NOLINTEND(*-pointer-arithmetic, *-magic-numbers)