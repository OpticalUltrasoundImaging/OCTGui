#pragma once

#include "Common.hpp"
#include "OCTRecon.hpp"
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QWidget>

namespace OCT {

class OCTReconParamsController : public QWidget {
public:
  OCTReconParamsController() {
    auto *layout = new QGridLayout;
    setLayout(layout);

    const auto makeQSpinBox =
        [this]<typename T>(T &value, const std::pair<int, int> &range,
                           const int step = 1) {
          auto *spinBox = new QSpinBox;
          spinBox->setRange(range.first, range.second);
          spinBox->setValue(static_cast<int>(value));
          spinBox->setSingleStep(step);
          connect(spinBox, &QSpinBox::valueChanged, this, [&](int newValue) {
            value = static_cast<T>(newValue);
            this->_paramsUpdatedInternal();
          });
          return spinBox;
        };

    const auto makeLabeledSpinbox =
        [this, &makeQSpinBox]<typename T>(
            QGridLayout *layout, int row, const QString &name,
            const QString &desc, const QString &suffix, T &value,
            const std::pair<int, int> &range, const int step = 1) {
          auto *label = new QLabel(name);
          label->setToolTip(desc);
          layout->addWidget(label, row, 0);

          auto *sp = makeQSpinBox(value, range, step);
          sp->setSuffix(suffix);
          layout->addWidget(sp, row, 1);

          updateGuiFromParamsCallbacks.emplace_back([this, sp, &value] {
            QSignalBlocker blocker(sp);
            sp->setValue(value);
          });

          return std::tuple{label, sp};
        };

    int i = 0;

    // NOLINTBEGIN(*-magic-numbers)
    {
      auto &value = m_params.n_splits;

      auto *label = new QLabel("Splits");
      label->setToolTip("No. of splits for split-spectrum OCT");
      layout->addWidget(label, i, 0);

      auto *spinBox = new QSpinBox;
      spinBox->setRange(1, 5);
      spinBox->setValue(static_cast<int>(value));
      spinBox->setSingleStep(1);
      connect(spinBox, &QSpinBox::valueChanged, this, [&](int newValue) {
        const double fct =
            static_cast<double>(value) / static_cast<double>(newValue);
        value = newValue;

        // When `n_splits` changes, update `imageDepth` and padTop
        qDebug() << "newValue" << newValue << "fct" << fct;
        qDebug() << "old imageDepth" << m_params.imageDepth;
        m_params.imageDepth =
            static_cast<int>(std::round(fct * m_params.imageDepth));
        qDebug() << "new imageDepth" << m_params.imageDepth;
        qDebug() << "old padTop" << m_params.padTop;
        m_params.padTop = static_cast<int>(std::round(fct * m_params.padTop));
        qDebug() << "new padTop" << m_params.padTop;

        this->_paramsUpdatedInternal();
      });

      spinBox->setSuffix("");
      layout->addWidget(spinBox, i++, 1);

      updateGuiFromParamsCallbacks.emplace_back([this, spinBox, &value] {
        QSignalBlocker blocker(spinBox);
        spinBox->setValue(value);
      });
    }

    makeLabeledSpinbox(layout, i++, "Image depth", "Height of rect image", {},
                       m_params.imageDepth, {100, 1000});

    makeLabeledSpinbox(
        layout, i++, "Brightness",
        "20 * log10(X) + Brightness. "
        "In the old software, the result of the 6144-point FFT is not "
        "normalized, and the default brightness was -60. "
        "With correction, dividing the FFT by 6144, the old default brightness "
        "value is approximately 17.",
        {}, m_params.brightness, {0, 50});

    makeLabeledSpinbox(layout, i++, "Contrast", "Multiplier after 20*log10(X).",
                       {}, m_params.contrast, {0, 15});

    makeLabeledSpinbox(layout, i++, "Pad top",
                       "Padding top (pixels) before polar transform.", "px",
                       m_params.padTop, {0, 625});

    auto [label, offsetSpinbox] = makeLabeledSpinbox(
        layout, i++, "Manual offset",
        "Manually change the rotation offset to rotate the image once", {},
        m_params.additionalOffset, {-1000, 1000});
    m_offsetSpinbox = offsetSpinbox; // NOLINT(*initializer)
    // NOLINTEND(*-magic-numbers)
  }

  [[nodiscard]] auto params() const { return m_params; }

  void clearOffset() {
    m_params.additionalOffset = 0;
    m_offsetSpinbox->setValue(0);
  }

private:
  OCTReconParams<Float> m_params{};
  std::vector<std::function<void()>> updateGuiFromParamsCallbacks;

  void updateGuiFromParams() {
    for (const auto &f : updateGuiFromParamsCallbacks) {
      f();
    }
  }

  QSpinBox *m_offsetSpinbox{};

  void _paramsUpdatedInternal() { updateGuiFromParams(); }
};

} // namespace OCT
