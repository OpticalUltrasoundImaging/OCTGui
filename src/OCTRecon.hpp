#pragma once

#include "fftw.hpp"
#include <cassert>
#include <fftw3.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <span>
#include <vector>

// NOLINTBEGIN(*-pointer-arithmetic, *-magic-numbers)

namespace OCT {

namespace fs = std::filesystem;

template <typename T>
concept Floating = std::is_same_v<T, double> || std::is_same_v<T, float>;

template <Floating T> std::vector<T> getHamming(int n) {
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

template <Floating T> struct ScanConversionParams {
  T contrast;
  T brightness;
};

template <Floating T>
[[nodiscard]] cv::Mat_<uint8_t>
reconBscan(const Calibration<T> &calib, const std::span<const uint16_t> fringe,
           size_t ALineSize, const int imageDepth = 624,
           ScanConversionParams<T> conversionParams = ScanConversionParams<T>{
               .contrast = 9., .brightness = -57.}) {

  assert((fringe.size() % ALineSize) == 0);
  const auto nLines = fringe.size() / ALineSize;

  // cv::Mat constructor takes (height, width)
  cv::Mat_<uint8_t> mat(nLines, imageDepth);

  const auto win = getHamming<T>(ALineSize);

  const auto contrast = conversionParams.contrast;
  const auto brightness = conversionParams.brightness;

  std::vector<T> alineBuf(ALineSize);
  std::vector<T> linearKFringe(ALineSize);

  auto &fft = fftw::EngineR2C1D<T>::get(ALineSize);

  const cv::Range range(0, nLines);
  // cv::parallel_for_(range, [&](cv::Range range) {
  fftw::R2CBuffer<T> fftBuf(ALineSize);

  for (int j = range.start; j < range.end; ++j) {
    const auto offset = j * ALineSize;

    // 1. Subtract background
    for (int i = 0; i < ALineSize; ++i) {
      alineBuf[i] = fringe[offset + i] - calib.background[i];
    }

    // 2. Interpolate phase calibration data
    for (int i = 0; i < ALineSize - 1; ++i) {
      const auto idx = calib.phaseCalib[i].idx;
      const auto unit = calib.phaseCalib[idx];
      const auto l_coeff = unit.l_coeff;
      const auto r_coeff = unit.r_coeff;
      linearKFringe[i] = alineBuf[idx] * l_coeff + alineBuf[idx + 1] * r_coeff;
    }

    // 3. FFT
    // Window
    for (int i = 0; i < ALineSize; ++i) {
      fftBuf.in[i] = win[i] * linearKFringe[i];
    }
    fft.forward(fftBuf.in, fftBuf.out);

    // 4. Copy result into image
    uint8_t *outptr = mat.ptr(j);
    for (size_t i = 0; i < imageDepth; ++i) {
      const auto ro = fftBuf.out[i][0];
      const auto io = fftBuf.out[i][1];

      T val = ro * ro + io * io;
      val = contrast * (10 * log10(val) + brightness);
      val = val < 0 ? 0 : val > 255 ? 255 : val;
      outptr[i] = static_cast<uint8_t>(val);
    }
  }
  // });

  mat = mat.t();

  // TODO crop to theoretical aline number

  // TODO align Bscans

  return mat;
}

} // namespace OCT

// NOLINTEND(*-pointer-arithmetic, *-magic-numbers)