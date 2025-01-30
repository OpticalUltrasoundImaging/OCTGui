#pragma once

#include "Common.hpp"
#include <fftconv/aligned_vector.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>

namespace OCT {

namespace fs = std::filesystem;

template <typename T>
void readTextFileToArray(const fs::path &filename, std::span<T> dst) {
  std::ifstream ifs(filename);
  if (ifs.is_open()) {
    T val{};
    size_t i{};

    while ((ifs >> val) && i < dst.size()) {
      dst[i++] = val;
    }

    if (ifs.eof()) {
      // std::cout << "Reached end of file " << filename << '\n';
    } else if (ifs.fail()) {
      std::cerr << "Failed to read value in file " << filename << '\n';
    } else if (ifs.bad()) {
      std::cerr << "Critical I/O error occured in file " << filename << '\n';
    }
  }
}

template <typename T>
void writeArrayToTextFile(const fs::path &filename,
                          const std::span<const T> src) {
  std::ofstream ofs(filename);
  if (ofs.is_open()) {
    for (const auto val : src) {
      if (!(ofs << val << '\n')) {
        break;
      }
    }

    if (ofs.fail()) {
      std::cerr << "Failed to write value to file " << filename << '\n';
    } else if (ofs.bad()) {
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
  fftconv::AlignedVector<T> background;
  fftconv::AlignedVector<phaseCalibUnit<T>> phaseCalib;

  Calibration(int n_samples, const fs::path &backgroundFile,
              const fs::path &phaseFile)
      : background(n_samples), phaseCalib(n_samples) {
    readTextFileToArray<T>(backgroundFile, background);
    readTextFileToArray<phaseCalibUnit<T>>(phaseFile, phaseCalib);
  }

  static std::shared_ptr<Calibration<T>>
  fromCalibDir(int n_samples, const fs::path &calibDir) {
    const auto backgroundFile = calibDir / "SSOCTBackground.txt";
    const auto phaseFile = calibDir / "SSOCTCalibration180MHZ.txt";
    if (fs::exists(backgroundFile) && fs::exists(phaseFile)) {
      return std::make_shared<Calibration>(n_samples, backgroundFile,
                                           phaseFile);
    }
    return nullptr;
  }

  void saveToNewCalibDir(const fs::path &newCalibDir) const {
    fs::create_directory(newCalibDir);

    const auto backgroundFile = newCalibDir / "SSOCTBackground.txt";
    const auto phaseFile = newCalibDir / "SSOCTCalibration180MHZ.txt";

    writeArrayToTextFile(backgroundFile, background);
    writeArrayToTextFile(phaseFile, phaseCalib);
  }
};

} // namespace OCT