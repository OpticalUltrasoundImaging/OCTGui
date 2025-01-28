#pragma once

#include "Common.hpp"
#include "phasecorr.hpp"
#include "timeit.hpp"
#include <cassert>
#include <fftconv/aligned_vector.hpp>
#include <fftconv/fftw.hpp>
#include <fftw3.h>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <numbers>
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/scalable_allocator.h>
#include <opencv2/core.hpp>
#include <opencv2/core/base.hpp>
#include <opencv2/opencv.hpp>
#include <span>
#include <tbb/parallel_for.h>
#include <tbb/scalable_allocator.h>

// NOLINTBEGIN(*-pointer-arithmetic, *-magic-numbers, *-reinterpret-cast)

namespace OCT {

namespace fs = std::filesystem;

template <typename T>
void readTextFileToArray(const fs::path &filename, std::span<T> dst) {
  std::ifstream ifs(filename);
  if (ifs.is_open()) {
    T val{};
    size_t i{};

    while (ifs >> val && i < dst.size()) {
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
};

template <Floating T> struct OCTReconParams {
  int imageDepth = 624;

  // Split spectrum OCT FFT
  int n_splits = 1;

  // Conversion params
  int contrast = 9;
  // In the old software, the result of the 6144-point FFT is not normalized,
  // and the default log shift factor (brightness) was -60. With correction,
  // dividing the FFT by 6144, the old default brightness value is approximately
  // 17.
  int brightness = 18;

  // Padding (top) for radial images
  int padTop = 300;

  // Clear pixels at the top of the rect image (DC offset)
  int clearTop = 20;

  // Change the rotation of the image
  int additionalOffset = 0;
};

template <typename T, typename Tout = T>
void logCompress(const std::span<Tout> out,
                 const std::span<const fftw::Complex<T>> cx, T contrast,
                 T brightness) {
  const T fct = 1.0 / cx.size();
  const T fct2 = 20 * log10(fct); // 20 because fct is not squared
  for (size_t i = 0; i < out.size(); ++i) {
    const T ro = cx[i][0];
    const T io = cx[i][1];
    T val = ro * ro + io * io;
    // Note the 10 * log10 is because val is the square
    val = contrast * (10 * log10(val) + brightness + fct2);
    out[i] = std::clamp<T>(val, 0, 255);
  }
}

template <typename T, typename Tout = T>
void logCompress_add(const std::span<Tout> out,
                     const std::span<const fftw::Complex<T>> inCx, T contrast,
                     T brightness, size_t offsetTop = 0) {
  assert(out.size() <= inCx.size());
  const T fct = 1.0 / inCx.size();
  const T fct2 = 20 * log10(fct); // 20 because fct is not squared
  for (size_t i = offsetTop; i < out.size(); ++i) {
    const T ro = inCx[i][0];
    const T io = inCx[i][1];
    T val = ro * ro + io * io;
    // Note the 10 * log10 is because val is squared
    val = contrast * (10 * log10(val) + brightness + fct2);
    out[i] += std::clamp<T>(val, 0, 255);
  }
}

inline int getDistortionOffset(const cv::Mat &mat, int theoryWidth,
                               int NumAlines) {
  constexpr int additionalCorrWidth = 0;
  const int corrWidth = NumAlines - theoryWidth + additionalCorrWidth;
  auto firstStrip = mat(cv::Rect(0, 0, corrWidth, mat.rows));
  auto lastStrip =
      mat(cv::Rect(theoryWidth - additionalCorrWidth, 0, corrWidth, mat.rows));
  firstStrip.convertTo(firstStrip, CV_32F);
  lastStrip.convertTo(lastStrip, CV_32F);

  // {
  //   cv::Mat firstStripDebug, lastStripDebug;
  //   firstStrip.convertTo(firstStripDebug, CV_8U);
  //   lastStrip.convertTo(lastStripDebug, CV_8U);
  //   cv::imshow("firstStrip", firstStripDebug);
  //   cv::imshow("lastStrip", lastStripDebug);
  // }

  return static_cast<int>(std::round(
      cvMod::phaseCorrelate(firstStrip, lastStrip).x - additionalCorrWidth));
}

inline void shiftXCircular(const cv::Mat &src, cv::Mat &dst, int shiftX) {
  // Normalize shift to positive range
  int width = src.cols;
  int shift = (shiftX % width + width) % width;

  // Create dst with the same size and type as src
  dst.create(src.size(), src.type());

  // Handle wrapping using modular arithmetic
  src(cv::Rect(0, 0, width - shift, src.rows))
      .copyTo(dst(cv::Rect(shift, 0, width - shift, src.rows)));

  src(cv::Rect(width - shift, 0, shift, src.rows))
      .copyTo(dst(cv::Rect(0, 0, shift, src.rows)));
}

template <typename T> inline void circshift(cv::Mat_<T> &mat, int idx) {
  assert(mat.isContinuous());
  idx = (idx + mat.cols) % mat.cols;
  const int depth = mat.channels();
  const int cols = mat.cols * depth;
  idx *= depth;
  for (int j = 0; j < mat.rows; ++j) {
    auto *ptr = mat.template ptr<T>(j);
    std::rotate(ptr, ptr + idx, ptr + cols);
  }
}

template <Floating T> auto getHamming(int n) {
  fftconv::AlignedVector<T> win(n);
  constexpr auto pi = std::numbers::pi_v<T>;
  for (int i = 0; i < n; ++i) {
    win[i] = 0.54 - 0.46 * std::cos(2 * pi * i / n);
  }
  return win;
}

/**
Original impl. without split spectrum
 */
template <Floating T>
[[nodiscard]] cv::Mat_<uint8_t>
reconBscan(const Calibration<T> &calib, const std::span<const uint16_t> fringe,
           const size_t ALineSize, const OCTReconParams<T> &params = {}) {

  assert((fringe.size() % ALineSize) == 0);
  const auto nLines = fringe.size() / ALineSize;

  const auto win = getHamming<T>(ALineSize);
  const auto contrast = params.contrast;
  const auto brightness = params.brightness;
  const size_t imageDepth = params.imageDepth;

  // cv::Mat constructor takes (height, width)
  cv::Mat_<T> mat(nLines, imageDepth);

  const auto &fft = fftw::EngineR2C1D<T>::get(ALineSize);

  tbb::blocked_range<size_t> range(0, nLines);
  tbb::parallel_for(range, [&](const tbb::blocked_range<size_t> &range) {
    fftw::R2CBuffer<T> fftBuf(ALineSize);
    std::vector<T, tbb::scalable_allocator<T>> alineBuf(ALineSize);
    std::vector<T, tbb::scalable_allocator<T>> linearKFringe(ALineSize);

    for (size_t j = range.begin(); j < range.end(); ++j) {
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
        linearKFringe[i] =
            alineBuf[idx] * l_coeff + alineBuf[idx + 1] * r_coeff;
      }

      // 3. FFT
      // Window
      for (int i = 0; i < ALineSize; ++i) {
        fftBuf.in[i] = win[i] * linearKFringe[i];
      }
      fft.forward(fftBuf.in, fftBuf.out);

      // 4. Copy result into image
      T *outptr = reinterpret_cast<T *>(mat.ptr(j));
      logCompress<T>({outptr, imageDepth}, {fftBuf.out, ALineSize}, contrast,
                     brightness);
    }
  });

  mat = mat.t();

  // Distortion correction and resize to theoretical aline number
  {
    TimeIt timeit;

    size_t theoreticalALines = nLines;
    if (nLines == 2500) {
      // theoreticalALines = 2234;
      // Don't need distortion correction for the ex vivo probe.
    } else if (nLines == 2200) {
      theoreticalALines = 2000;

      const cv::Size targetSize(theoreticalALines, mat.rows);
      const int distOffset =
          getDistortionOffset(mat, theoreticalALines, nLines);
      cv::resize(mat(cv::Rect(0, 0, theoreticalALines + distOffset, mat.rows)),
                 mat, targetSize);
    }

    // fmt::println("Distortion correction elapsed: {} ms", timeit.get_ms());
  }

  // Align Bscans
  {
    TimeIt timeit;
    static cv::Mat_<T> prevMat;
    if (prevMat.cols == mat.cols && prevMat.rows == mat.rows) {
      int alignOffset = std::round(cvMod::phaseCorrelate(prevMat, mat).x);
      circshift(mat, alignOffset + params.additionalOffset);
    }
    mat.copyTo(prevMat);

    // fmt::println("Align correction elapsed: {} ms", timeit.get_ms());
  }

  cv::Mat_<uint8_t> outmat;
  mat.convertTo(outmat, CV_8U);
  return mat;
}

/**
Split the `n` point sampled spectral fringe to `n_splits`, using size `n /
n_splits` FFTs instead of size `n` FFTs, and average the result
 */
template <Floating T>
[[nodiscard]] cv::Mat_<uint8_t> reconBscan_splitSpectrum(
    const Calibration<T> &calib, const std::span<const uint16_t> fringe,
    const size_t ALineSize, const OCTReconParams<T> &params = {}) {

  assert((fringe.size() % ALineSize) == 0);
  const auto nLines = fringe.size() / ALineSize;

  const size_t n_splits = params.n_splits;
  const size_t splitSize = ALineSize / n_splits;

  const auto win = getHamming<T>(splitSize);
  const auto contrast = params.contrast;
  const auto brightness = params.brightness;
  const size_t imageDepth = params.imageDepth;

  // cv::Mat constructor takes (height, width)
  // std::vector<cv::Mat_<T>> mats(n_splits);
  // for (int i_split = 0; i_split < n_splits; ++i_split) {
  //   mats[i_split] = cv::Mat_<T>(nLines, imageDepth);
  // }
  cv::Mat_<T> mat = cv::Mat_<T>::zeros(nLines, imageDepth);

  const auto &fft = fftw::EngineR2C1D<T>::get(splitSize);

  tbb::blocked_range<size_t> range(0, nLines);
  tbb::parallel_for(range, [&](const tbb::blocked_range<size_t> &range) {
    fftw::R2CBuffer<T> fftBuf(ALineSize);
    std::vector<T, tbb::scalable_allocator<T>> alineBuf(ALineSize);
    std::vector<T, tbb::scalable_allocator<T>> linearKFringe(ALineSize);

    for (size_t j = range.begin(); j < range.end(); ++j) {
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
        linearKFringe[i] =
            alineBuf[idx] * l_coeff + alineBuf[idx + 1] * r_coeff;
      }

      for (int i_split = 0; i_split < n_splits; ++i_split) {
        // 3. Windowed FFT over splits
        const int offset = i_split * splitSize;
        for (int i = 0; i < splitSize; ++i) {
          fftBuf.in[i] = win[i] * linearKFringe[offset + i];
        }
        fft.forward(fftBuf.in, fftBuf.out);

        // 4. Copy result into image
        T *outptr = reinterpret_cast<T *>(mat.ptr(j));
        logCompress_add<T>({outptr, imageDepth}, {fftBuf.out, splitSize},
                           contrast, brightness, params.clearTop);
      }
    }
  });

  mat = mat.t();

  // Distortion correction and resize to theoretical aline number
  {
    TimeIt timeit;

    size_t theoreticalALines = nLines;
    if (nLines == 2500) {
      // theoreticalALines = 2234;
      // Don't need distortion correction for the ex vivo probe.
    } else if (nLines == 2200) {
      theoreticalALines = 2000;

      const cv::Size targetSize(theoreticalALines, mat.rows);
      const int distOffset =
          getDistortionOffset(mat, theoreticalALines, nLines);
      cv::resize(mat(cv::Rect(0, 0, theoreticalALines + distOffset, mat.rows)),
                 mat, targetSize);
    }

    // fmt::println("Distortion correction elapsed: {} ms", timeit.get_ms());
  }

  // Align Bscans
  {
    TimeIt timeit;
    static cv::Mat_<T> prevMat;
    if (prevMat.cols == mat.cols && prevMat.rows == mat.rows) {
      int alignOffset = std::round(cvMod::phaseCorrelate(prevMat, mat).x);
      circshift(mat, alignOffset + params.additionalOffset);
    }
    mat.copyTo(prevMat);

    // fmt::println("Align correction elapsed: {} ms", timeit.get_ms());
  }

  cv::Mat_<uint8_t> outmat;
  mat.convertTo(outmat, CV_8U);
  return mat;
}

inline void makeRadialImage(const cv::Mat_<uint8_t> &in, cv::Mat_<uint8_t> &out,
                            int padTop = 0) {

  const int dim = std::min(in.rows, in.cols);

  // const cv::Size dsize{dim, dim};
  // const double radius = dim / 2;
  // const cv::Point2f center(radius, radius);

  const cv::Size dsize{dim * 2, dim * 2};
  const double radius = dim;
  const auto radiusf = static_cast<float>(radius);
  const cv::Point2f center(radiusf, radiusf);

  const int flags = cv::WARP_FILL_OUTLIERS + cv::WARP_INVERSE_MAP;

  if (padTop != 0) {
    cv::copyMakeBorder(in, out, padTop, 0, 0, 0, cv::BORDER_CONSTANT);
    cv::warpPolar(out.t(), out, dsize, center, radius,
                  flags); // linear Polar, 3
  } else {
    cv::warpPolar(in.t(), out, dsize, center, radius,
                  flags); // linear Polar, 3
  }

  cv::flip(out, out, 1);
}

} // namespace OCT

// NOLINTEND(*-pointer-arithmetic, *-magic-numbers, *-reinterpret-cast)
