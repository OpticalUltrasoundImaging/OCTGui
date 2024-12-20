#pragma once

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsView>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <Qt>

class ImageDisplay : public QGraphicsView {
  Q_OBJECT;

public:
  ImageDisplay() : m_Scene(new QGraphicsScene) {
    setBackgroundBrush(QBrush(Qt::black));
    setAlignment(Qt::AlignCenter);

    setMouseTracking(true);
    setCursor(Qt::CrossCursor);

    viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
    grabGesture(Qt::PinchGesture);

    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setScene(m_Scene);
  }

  void imshow(const QPixmap &pixmap) {
    m_Pixmap = pixmap;
    if (m_PixmapItem != nullptr) {
      m_Scene->removeItem(m_PixmapItem);
      delete m_PixmapItem;
    }
    m_PixmapItem = m_Scene->addPixmap(m_Pixmap);
    m_PixmapItem->setZValue(-1);

    // TODO resize and zoom

    // TODO overlay
  }
  void imshow(const QImage &img) { imshow(QPixmap::fromImage(img)); }

private:
  QGraphicsScene *m_Scene;
  QPixmap m_Pixmap;
  QGraphicsItem *m_PixmapItem{};

  double m_scaleFactor{1.0};
  double m_scaleFactorMin{1.0};
};