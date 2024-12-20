#pragma once

#include <QCursor>
#include <QHBoxLayout>
#include <QSlider>
#include <QString>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>

namespace OCT {

class FrameController : public QWidget {
  Q_OBJECT
public:
  FrameController() : m_slider(new QSlider) {
    m_slider->setMinimum(0);
    m_slider->setMaximum(0);
    m_slider->setTickInterval(1);
    m_slider->setTickPosition(QSlider::TickPosition::TicksBelow);
    m_slider->setOrientation(Qt::Horizontal);

    // GUI
    // ---
    auto *layout = new QHBoxLayout;
    setLayout(layout);
    layout->addWidget(m_slider);

    // Bind
    connect(m_slider, &QSlider::sliderPressed, this, [&] {
      QToolTip::showText(QCursor::pos(), QString::number(m_slider->value()));
    });
    connect(m_slider, &QSlider::sliderMoved, this, [&] {
      QToolTip::showText(QCursor::pos(), QString::number(m_slider->value()));
    });
    connect(m_slider, &QSlider::sliderReleased, this, [&] {
      const auto pos = m_slider->value();
      QToolTip::showText(QCursor::pos(), QString::number(pos));
      emit posChanged(pos);
    });
  }

  void setSize(size_t size) {
    m_size = size;
    m_slider->setMaximum(size);
  }
  void setPos(size_t pos) { m_pos = pos; }
  [[nodiscard]] size_t size() const { return m_size; }
  [[nodiscard]] size_t pos() const { return m_pos; }

public slots:
  void nextNoEmit() {
    const auto i = pos();
    if (i < size() - 1) {
      m_slider->setValue(i + 1);
    }
  }
  void prevNoEmit() {
    const auto i = pos();
    if (i > 0) {
      m_slider->setValue(i - 1);
    }
  }
  void next() {
    nextNoEmit();
    emit posChanged(pos());
  }
  void prev() {
    prevNoEmit();
    emit posChanged(pos());
  }

signals:
  void posChanged(size_t pos);

private:
  size_t m_size{};
  size_t m_pos{};

  QSlider *m_slider;
};

} // namespace OCT