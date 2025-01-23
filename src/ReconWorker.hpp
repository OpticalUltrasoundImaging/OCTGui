#pragma once

#include "Common.hpp"
#include "ExportSettings.hpp"
#include "ImageDisplay.hpp"
#include "OCTRecon.hpp"
#include "RingBuffer.hpp"
#include <QImage>
#include <QObject>
#include <QPixmap>
#include <atomic>
#include <cstddef>
#include <qobjectdefs.h>
#include <qpixmap.h>
#include <utility>

namespace OCT {

inline QPixmap matToQPixmap(const cv::Mat_<uint8_t> &mat) {
  if (mat.empty()) {
    return {};
  }

  QImage img(mat.data, mat.cols, mat.rows, static_cast<qsizetype>(mat.step),
             QImage::Format_Grayscale8);
  return QPixmap::fromImage(img);
}

class ReconWorker : public QObject {
  Q_OBJECT;

public:
  explicit ReconWorker(std::shared_ptr<RingBuffer<OCTData<Float>>> buffer,
                       size_t ALineSize, ImageDisplay *imageDisplay)
      : m_ringBuffer(std::move(buffer)), ALineSize(ALineSize),
        m_imageDisplay(imageDisplay) {}

Q_SIGNALS:
  void statusMessage(QString msg);

public Q_SLOTS:
  void setCalibration(std::shared_ptr<Calibration<Float>> calibration) {
    this->m_calib = std::move(calibration);
  }
  void setALineSize(size_t ALineSize) { this->ALineSize = ALineSize; }
  void setShouldStop(bool shouldStop) { this->shouldStop = shouldStop; }

  void setParams(OCTReconParams<Float> params) { m_params = params; }
  void setExportSettings(const ExportSettings &settings) {
    m_exportSettings = settings;
  }

  void start() {
    assert(m_ringBuffer != nullptr);

    while (!shouldStop) {

      m_ringBuffer->consume([this](std::shared_ptr<OCTData<Float>> &dat) {
        if (m_calib == nullptr || dat == nullptr) {
          return;
        }

        TimeIt timeit;
        float elapsedRecon{};
        {
          TimeIt timeitRecon;
          dat->imgRect = reconBscan_splitSpectrum<Float>(*m_calib, dat->fringe,
                                                         ALineSize, m_params);
          elapsedRecon = timeitRecon.get_ms();
        }

        float elapsedRadial{};
        {
          TimeIt timeit;
          makeRadialImage(dat->imgRect, dat->imgRadial, m_params.padTop);
          elapsedRadial = timeit.get_ms();
        }

        if (m_exportSettings.saveImages) {
          exportImages(*dat);
        }

        makeCombinedImage(*dat);

        // Update image display
        const QPixmap combinedPixmap = matToQPixmap(dat->imgCombined);
        QMetaObject::invokeMethod(m_imageDisplay, &ImageDisplay::imshow,
                                  combinedPixmap);
        QMetaObject::invokeMethod(m_imageDisplay->overlay(),
                                  &ImageOverlay::setProgress, dat->i, -1);

        // Status message
        const auto elapsedTotal = timeit.get_ms();
        const auto msg =
            fmt::format("Loaded frame {}, recon {:.3f} ms, total {:.3f} ms",
                        dat->i, elapsedRecon, elapsedTotal);
        Q_EMIT statusMessage(QString::fromStdString(msg));
      });
    }
  }

  void exportImages(const OCTData<Float> &dat) const {
    {
      auto outpath =
          m_exportSettings.exportDir / fmt::format("rect-{:03}.tiff", dat.i);
      cv::imwrite(outpath.string(), dat.imgRect);
    }
    {
      auto outpath =
          m_exportSettings.exportDir / fmt::format("radial-{:03}.tiff", dat.i);
      cv::imwrite(outpath.string(), dat.imgRadial);
    }
  }

  static void makeCombinedImage(OCTData<Float> &dat) {

    dat.imgCombined = cv::Mat_<uint8_t>(dat.imgRadial.rows,
                                        dat.imgRadial.cols + dat.imgRect.cols);

    // Copy radial to left side
    dat.imgRadial.copyTo(dat.imgCombined(
        cv::Rect(0, 0, dat.imgRadial.cols, dat.imgRadial.rows)));

    // Copy rect to top right
    dat.imgRect.copyTo(dat.imgCombined(
        cv::Rect(dat.imgRadial.cols, 0, dat.imgRect.cols, dat.imgRect.rows)));

    // Clear bottom right
    dat.imgCombined(cv::Rect(dat.imgRadial.cols, dat.imgRect.rows,
                             dat.imgRect.cols,
                             dat.imgCombined.rows - dat.imgRect.rows))
        .setTo(0);
  }

private:
  std::atomic<bool> shouldStop{false};

  std::shared_ptr<RingBuffer<OCTData<Float>>> m_ringBuffer;
  std::shared_ptr<Calibration<Float>> m_calib;
  size_t ALineSize;
  OCTReconParams<Float> m_params;
  ExportSettings m_exportSettings;

  ImageDisplay *m_imageDisplay;
};

} // namespace OCT
