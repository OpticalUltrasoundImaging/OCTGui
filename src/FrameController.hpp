#pragma once

#include <QAction>
#include <QCursor>
#include <QHBoxLayout>
#include <QMenu>
#include <QSlider>
#include <QString>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>
#include <qnamespace.h>

namespace OCT {

class FrameController : public QWidget {
  Q_OBJECT
public:
  FrameController()
      : m_slider(new QSlider), m_menu(new QMenu("Frame")),
        m_actNext(new QAction("Next frame")),
        m_actPrev(new QAction("Prev frame")) {
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

    // Actions and menu
    m_menu->addAction(m_actNext);
    m_menu->addAction(m_actPrev);

    // Enable `prev` and `next` actions to seek the previous or next frame
    connect(m_actPrev, &QAction::triggered, this, &FrameController::prev);
    connect(m_actNext, &QAction::triggered, this, &FrameController::next);

    // Keyboard shortcuts for `prev` and `next`
    m_actPrev->setShortcut({Qt::Key_Comma});
    m_actNext->setShortcuts({{Qt::Key_Period}, {Qt::Key_Space}});
  }

  auto menu() { return m_menu; }

  void setSize(size_t size) { m_slider->setMaximum(size - 1); }
  void setPos(size_t pos) { m_slider->setValue(pos); }
  [[nodiscard]] size_t size() const { return m_slider->maximum(); }
  [[nodiscard]] size_t pos() const { return m_slider->value(); }

public slots:
  void nextNoEmit() {
    const auto i = pos();
    if (i < size() - 1) {
      setPos(i + 1);
    }
  }
  void prevNoEmit() {
    const auto i = pos();
    if (i > 0) {
      setPos(i - 1);
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
  QSlider *m_slider;

  QMenu *m_menu;
  QAction *m_actNext;
  QAction *m_actPrev;
};

} // namespace OCT