#pragma once

#include "Common.hpp"
#include "OCTRecon.hpp"
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QWidget>
#include <functional>

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
            // this->_paramsUpdatedInternal();
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

    makeLabeledSpinbox(layout, i++, "Image depth", "", {}, m_params.imageDepth,
                       {200, 1000});

    makeLabeledSpinbox(layout, i++, "Brightness", "", {}, m_params.brightness,
                       {-70, 0});

    makeLabeledSpinbox(layout, i++, "Contrast", "", {}, m_params.contrast,
                       {0, 15});

    auto [label, offsetSpinbox] =
        makeLabeledSpinbox(layout, i++, "Offset", "", {},
                           m_params.additionalOffset, {-1000, 1000});
    m_offsetSpinbox = offsetSpinbox; // NOLINT(*initializer)

    makeLabeledSpinbox(layout, i++, "Pad top", "", "px", m_params.padTop,
                       {0, 625});

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

  void updateGuiFromParams();

  QSpinBox *m_offsetSpinbox{};
};

} // namespace OCT
