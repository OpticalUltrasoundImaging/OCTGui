#pragma once

#include <QGridLayout>
#include <QLabel>
#include <QSize>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>

class OverlayWidget : public QWidget {
public:
  explicit OverlayWidget(QWidget *parent)
      : QWidget(parent), m_topLeft(new QLabel), m_topRight(new QLabel),
        m_bottomRight(new QLabel), m_bottomLeft(new QLabel),
        m_topLeftLayout(new QVBoxLayout), m_topRightLayout(new QVBoxLayout),
        m_bottomRightLayout(new QVBoxLayout),
        m_bottomLeftLayout(new QVBoxLayout) {
    // Make widget transparent to input and visuals
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                   Qt::WindowTransparentForInput);

    setStyleSheet("QLabel { color: white; border: none }");

    // UI
    // --
    auto *grid = new QGridLayout;
    setLayout(grid);

    // Top left
    m_topLeftLayout->setAlignment(Qt::AlignTop);
    grid->addLayout(m_topLeftLayout, 0, 0);
    // Top right
    m_topRightLayout->setAlignment(Qt::AlignTop);
    grid->addLayout(m_topRightLayout, 0, 1);
    // Bottom left
    m_bottomLeftLayout->setAlignment(Qt::AlignBottom);
    grid->addLayout(m_bottomLeftLayout, 0, 0);
    // Bottom right
    m_bottomRightLayout->setAlignment(Qt::AlignBottom);
    grid->addLayout(m_bottomRightLayout, 0, 1);
  }

  QVBoxLayout *topLeftLayout() { return m_topLeftLayout; }
  QVBoxLayout *topRightLayout() { return m_topRightLayout; }
  QVBoxLayout *bottomLeftLayout() { return m_bottomLeftLayout; }
  QVBoxLayout *bottomRightLayout() { return m_bottomRightLayout; }

private:
  QLabel *m_topLeft;
  QLabel *m_topRight;
  QLabel *m_bottomRight;
  QLabel *m_bottomLeft;

  QVBoxLayout *m_topLeftLayout;
  QVBoxLayout *m_topRightLayout;
  QVBoxLayout *m_bottomRightLayout;
  QVBoxLayout *m_bottomLeftLayout;
};

namespace OCT {

class ImageOverlay : public OverlayWidget {
public:
  explicit ImageOverlay(QWidget *parent)
      : OverlayWidget(parent), m_sequence(new QLabel), m_progress(new QLabel) {
    topLeftLayout()->addWidget(m_sequence);
    topLeftLayout()->addWidget(m_progress);
  }

  void setSequence(const QString &sequence) { m_sequence->setText(sequence); }
  void setSize(size_t size) {
    m_size = size;
    setProgress(m_idx, m_size);
  }
  void setIdx(size_t idx) {
    m_idx = idx;
    setProgress(m_idx, m_size);
  }
  void setProgress(size_t idx, size_t size) {
    m_progress->setText(QString("%1/%2").arg(idx).arg(size));
  }

  void clear() {
    m_sequence->clear();
    m_progress->clear();
    m_size = 0;
    m_idx = 0;
  }

private:
  QLabel *m_sequence{};

  QLabel *m_progress;
  size_t m_size{};
  size_t m_idx{};
};

} // namespace OCT