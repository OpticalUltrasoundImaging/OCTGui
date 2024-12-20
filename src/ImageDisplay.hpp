#pragma once

#include <QEvent>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsView>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTransform>
#include <QWheelEvent>
#include <Qt>
#include <qevent.h>
#include <qnamespace.h>

class ImageDisplay : public QGraphicsView {
  Q_OBJECT;

public:
  struct CursorState {
    bool leftButton{};
    bool middleButton{};
    bool rightButton{};
  };

  ImageDisplay() : m_Scene(new QGraphicsScene) {
    setBackgroundBrush(QBrush(Qt::black));

    setAlignment(Qt::AlignCenter);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

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

    // TODO overlay
  }
  void imshow(const QImage &img) { imshow(QPixmap::fromImage(img)); }

  void scaleToSize() {
    updateMinScaleFactor();
    m_scaleFactor = m_scaleFactorMin;
    updateTransform();
  }

protected:
  void wheelEvent(QWheelEvent *event) override {
    constexpr auto WHEEL_ZOOM_MODIFIER = Qt::ControlModifier;
    if (event->modifiers().testFlag(WHEEL_ZOOM_MODIFIER)) {
      event->accept();

      // Calculate scale factor
      const double numDeg = event->angleDelta().y() / 8.0;
      const double numSteps = numDeg / 15.0;
      constexpr double sensitivity = 0.1;
      const double scaleFactor = 1.0 - numSteps * sensitivity;
      const auto &evPos = event->position();
      // const auto pos = mapToScene(QPoint{(int)evPos.x(), (int)evPos.y()});
      m_scaleFactor = std::max(m_scaleFactor * scaleFactor, m_scaleFactorMin);

      updateTransform();
    } else {
      QGraphicsView::wheelEvent(event);
    }
  }

  void resizeEvent(QResizeEvent *event) override {
    updateMinScaleFactor();
    QGraphicsView::resizeEvent(event);
  }

  void mousePressEvent(QMouseEvent *event) override {
    if (event->button() == Qt::MiddleButton) {
      m_cursor.middleButton = true;
      panStartEvent(event);
    }
  }

  void mouseMoveEvent(QMouseEvent *event) override {
    if (m_cursor.middleButton) {
      panMoveEvent(event);
    }
  }

  void mouseReleaseEvent(QMouseEvent *event) override {
    if (m_cursor.middleButton) {
      m_cursor.middleButton = false;
      panEndEvent(event);
    }
  }

private:
  QGraphicsScene *m_Scene;
  QPixmap m_Pixmap;
  QGraphicsItem *m_PixmapItem{};

  double m_scaleFactor{1.0};
  double m_scaleFactorMin{1.0};
  QTransform m_transform;

  CursorState m_cursor;
  QPoint m_lastPanPoint{};
  QCursor m_lastPanCursor{};

  void updateTransform() {
    m_transform = QTransform();
    m_transform.scale(m_scaleFactor, m_scaleFactor);

    setTransformationAnchor(AnchorUnderMouse);
    setTransform(m_transform);
  }

  void updateMinScaleFactor() {
    if (m_Pixmap.isNull()) {
      return;
    }
    const auto w = width();
    const auto h = height();
    const auto pw = m_Pixmap.width();
    const auto ph = m_Pixmap.height();

    m_scaleFactorMin =
        qMin(w / static_cast<qreal>(pw), h / static_cast<qreal>(ph));
  }

  void panStartEvent(QMouseEvent *event) {
    event->accept();
    m_lastPanPoint = event->pos();
    m_lastPanCursor = cursor();
    setCursor(Qt::ClosedHandCursor);
  }
  void panMoveEvent(QMouseEvent *event) {
    event->accept();
    const auto delta = event->pos() - m_lastPanPoint;
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
    verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
    m_lastPanPoint = event->pos();
  }
  void panEndEvent(QMouseEvent *event) { setCursor(m_lastPanCursor); }
};