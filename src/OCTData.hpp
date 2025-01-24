#pragma once

#include "Common.hpp"
#include <fftconv/aligned_vector.hpp>
#include <opencv2/opencv.hpp>

namespace OCT {

template <Floating T> struct OCTData {
  fftconv::AlignedVector<uint16_t> fringe;
  size_t i{};

  cv::Mat_<uint8_t> imgRect;
  cv::Mat_<uint8_t> imgRadial;
  cv::Mat_<uint8_t> imgCombined;
};

} // namespace OCT