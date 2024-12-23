#pragma once

#include "Common.hpp"
#include "OCTRecon.hpp"
#include "RingBuffer.hpp"
#include <QImage>
#include <QObject>
#include <QPixmap>
#include <atomic>
#include <cstddef>
#include <utility>

namespace OCT {

inline QPixmap matToQPixmap(const cv::Mat_<uint8_t> &mat) {
  if (mat.empty()) {
    return {};
  }

  QImage img(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
  return QPixmap::fromImage(img);
}

class ReconWorker : public QObject {
  Q_OBJECT;

public:
  explicit ReconWorker(std::shared_ptr<RingBufferOfVec<uint16_t>> buffer,
                       size_t ALineSize)
      : ringBuffer(std::move(buffer)), ALineSize(ALineSize) {}

public slots:
  void setCalibration(std::shared_ptr<Calibration<Float>> calibration) {
    this->calibration = std::move(calibration);
  }
  void setALineSize(size_t ALineSize) { this->ALineSize = ALineSize; }
  void setShouldStop(bool shouldStop) { this->shouldStop = shouldStop; }

  void start() {
    assert(ringBuffer != nullptr);
    assert(calibration != nullptr);

    while (!shouldStop) {
      ringBuffer->consume([this](const std::span<const uint16_t> fringe) {
        auto mat = reconBscan(*calibration, fringe, ALineSize);
        emit completeOne(matToQPixmap(mat));
      });
    }
  }

signals:
  void completeOne(const QPixmap &pixmap);

private:
  std::shared_ptr<RingBufferOfVec<uint16_t>> ringBuffer;
  std::shared_ptr<Calibration<Float>> calibration;
  size_t ALineSize;
  std::atomic<bool> shouldStop{false};
};

} // namespace OCT
