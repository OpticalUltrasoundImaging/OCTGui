#pragma once

#include "Common.hpp"
#include "FileIO.hpp"
#include <QDebug>
#include <algorithm>
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
      ofs << val << '\n';
      if (!ofs) {
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

  friend std::istream &operator>>(std::istream &is, phaseCalibUnit<T> &x) {
    return is >> x.idx >> x.l_coeff >> x.r_coeff;
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const phaseCalibUnit<T> &x) {
    return os << x.idx << ' ' << x.l_coeff << ' ' << x.r_coeff << '\n';
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
    if (!fs::exists(newCalibDir)) {
      fs::create_directory(newCalibDir);
    }

    const auto backgroundFile = newCalibDir / "SSOCTBackground.txt";
    const auto phaseFile = newCalibDir / "SSOCTCalibration180MHZ.txt";

    writeArrayToTextFile<T>(backgroundFile, background);
    writeArrayToTextFile<phaseCalibUnit<T>>(phaseFile, phaseCalib);
  }

  // Read a data bin, calculate background from the first n frames.
  void updateBackgroundFromBinfile(const fs::path &path, int nFrames) {
    auto reader = DatFileReader::readBinFile(path);
    nFrames = std::min<size_t>(nFrames, reader.size());

    const auto samples = reader.samplesPerFrame() * nFrames;
    fftconv::AlignedVector<uint16_t> fringe(samples);

    if (const auto err = reader.read(0, nFrames, fringe)) {
      qDebug() << "While reading background bin, got" << err;
    } else {
      // Read successful
      const auto ALineSize = DatFileReader::ALineSize;
      const auto nLines = fringe.size() / ALineSize;

      // Compute average
      fftconv::AlignedVector<double> acc(ALineSize, 0);
      for (size_t j = 0; j < nLines; ++j) {
        const auto offset = j * ALineSize;
        for (size_t i = 0; i < ALineSize; ++i) {
          acc[i] += fringe[offset + i];
        }
      }

      const auto fct = 1.0 / static_cast<double>(nLines);
      for (auto &val : acc) {
        val *= fct;
      }

      // Update background
      std::copy(acc.begin(), acc.end(), background.begin());
    }
  }
};

} // namespace OCT