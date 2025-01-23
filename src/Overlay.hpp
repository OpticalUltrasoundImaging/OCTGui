#pragma once

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSize>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>
#include <qboxlayout.h>
#include <qlabel.h>

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
    auto *hlayout = new QHBoxLayout;
    auto *leftv = new QVBoxLayout;
    auto *rightv = new QVBoxLayout;
    hlayout->addLayout(leftv);
    hlayout->addLayout(rightv);
    setLayout(hlayout);

    leftv->addLayout(m_topLeftLayout);
    leftv->addLayout(m_bottomLeftLayout);
    rightv->addLayout(m_topRightLayout);
    rightv->addLayout(m_bottomRightLayout);

    m_topLeftLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_topRightLayout->setAlignment(Qt::AlignTop | Qt::AlignRight);
    m_bottomLeftLayout->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
    m_bottomRightLayout->setAlignment(Qt::AlignBottom | Qt::AlignRight);
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

class ImageOverlay : public OverlayWidget {
public:
  explicit ImageOverlay(QWidget *parent)
      : OverlayWidget(parent), m_sequence(new QLabel), m_filename(new QLabel),
        m_modality(new QLabel), m_progress(new QLabel), m_imageSize(new QLabel),
        m_zoom(new QLabel) {
    topLeftLayout()->addWidget(m_sequence);

    bottomLeftLayout()->addWidget(m_modality);
    bottomLeftLayout()->addWidget(m_progress);
    bottomLeftLayout()->addWidget(m_imageSize);

    bottomRightLayout()->addWidget(m_zoom);
  }

public Q_SLOTS:
  void setSequence(const QString &sequence) { m_sequence->setText(sequence); }
  void setFilename(const QString &name) { m_filename->setText(name); }

  void setModality(const QString &modality) { m_modality->setText(modality); };
  void setProgress(int idx, int size = -1) {
    if (size >= 0) {
      m_size = size;
    }
    m_progress->setText(QString("%1/%2").arg(idx).arg(m_size));
  }
  void setImageSize(const QSize &size) {
    m_imageSize->setText(
        QString("Slice: %1 x %2").arg(size.width()).arg(size.height()));
  }
  void setZoom(double zoom) {
    // NOLINTNEXTLINE(*-magic-numbers)
    m_zoom->setText(QString("Zoom: %1%").arg(static_cast<int>(zoom * 100)));
  }

  void clear() {
    m_sequence->clear();
    m_modality->clear();
    m_progress->clear();
    m_imageSize->clear();
    m_zoom->clear();
  }

private:
  // Top left
  QLabel *m_sequence{};
  QLabel *m_filename{};

  // Bottom left
  QLabel *m_modality;
  QLabel *m_progress;
  int m_size{};
  QLabel *m_imageSize;

  // Bottom right
  QLabel *m_zoom;
};
