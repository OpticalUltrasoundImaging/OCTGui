#pragma once

#include "Annotation/Annotation.hpp"
#include "Annotation/GeometryUtils.hpp"
#include "Annotation/GraphicsItems.hpp"
#include "Overlay.hpp"
#include <QEvent>
#include <QGestureEvent>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsView>
#include <QImage>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTransform>
#include <QWheelEvent>
#include <Qt>
#include <qnamespace.h>

class ImageDisplay : public QGraphicsView {
  Q_OBJECT;

public:
  struct CursorState {
    bool leftButton{};
    bool middleButton{};
    bool rightButton{};

    // Position in pixmap coord
    QPointF pos, startPos;

    [[nodiscard]] auto line() const { return QLineF(startPos, pos); }
    [[nodiscard]] auto rect() const {
      qreal x = qMin(pos.x(), startPos.x());
      qreal y = qMin(pos.y(), startPos.y());

      qreal w = qAbs(pos.x() - startPos.x());
      qreal h = qAbs(pos.y() - startPos.y());
      return QRectF(x, y, w, h);
    }
  };

  enum class CursorMode {
    Default = 0, // Delegate to QGraphicsView
    MeasureLine,
  };
  Q_ENUM(CursorMode)

  ImageDisplay()
      : m_Scene(new QGraphicsScene), m_overlay(new ImageOverlay(viewport())) {
    setBackgroundBrush(QBrush(Qt::black));

    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    setAlignment(Qt::AlignCenter);

    // Hide scrollbars
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Enable mouse tracking
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::StrongFocus);

    // Enable pinch gesture handling
    viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
    grabGesture(Qt::PinchGesture);

    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setScene(m_Scene);

    m_overlay->hide();
  }

  [[nodiscard]] auto overlay() { return m_overlay; }

  void imshow(const QPixmap &pixmap) {
    m_Pixmap = pixmap;
    if (m_PixmapItem != nullptr) {
      m_Scene->removeItem(m_PixmapItem);
      delete m_PixmapItem;
    }
    m_PixmapItem = m_Scene->addPixmap(m_Pixmap);
    m_PixmapItem->setZValue(-1);

    if (m_resetZoomOnNext) {
      scaleToSize();
      m_resetZoomOnNext = false;
    }

    m_overlay->setImageSize(pixmap.size());
    m_overlay->show();
  }
  void imshow(const QImage &img) { imshow(QPixmap::fromImage(img)); }

  void resetZoomOnNext() { m_resetZoomOnNext = true; }

  void scaleToSize() {
    updateMinScaleFactor();
    m_scaleFactor = m_scaleFactorMin;
    updateTransform();
  }

protected:
  bool event(QEvent *event) override {
    if (event->type() == QEvent::Gesture) {
      return gestureEvent(dynamic_cast<QGestureEvent *>(event));
    }
    return QGraphicsView::event(event);
  }

  void wheelEvent(QWheelEvent *event) override {
    constexpr auto WHEEL_ZOOM_MODIFIER = Qt::ControlModifier;
    if (event->modifiers().testFlag(WHEEL_ZOOM_MODIFIER)) {
      event->accept();

      // Calculate scale factor
      // https://wiki.qt.io/Smooth_Zoom_In_QGraphicsView
      const double numDeg = event->angleDelta().y() / 8.0;
      const double numSteps = numDeg / 15.0;
      constexpr double sensitivity = 0.1;
      const double scaleFactor = 1.0 - numSteps * sensitivity;
      m_scaleFactor = std::max(m_scaleFactor * scaleFactor, m_scaleFactorMin);

      updateTransform();
    } else {
      QGraphicsView::wheelEvent(event);
    }
  }

  void paintEvent(QPaintEvent *event) override {
    m_overlay->move(0, 0);
    QGraphicsView::paintEvent(event);
  }

  void resizeEvent(QResizeEvent *event) override {
    updateMinScaleFactor();
    QGraphicsView::resizeEvent(event);
    m_overlay->resize(viewport()->size());
  }

  void mousePressEvent(QMouseEvent *event) override {
    m_cursor.startPos = m_PixmapItem->mapFromScene(mapToScene(event->pos()));
    m_cursor.pos = m_cursor.startPos;

    if (event->button() == Qt::LeftButton) {
      m_cursor.leftButton = true;
      switch (m_cursorMode) {
      case CursorMode::Default:
        break;
      case CursorMode::MeasureLine:
        event->accept();

        const auto line = m_cursor.line();
        const auto color = Qt::white;

        if (m_currItem != nullptr) {
          m_Scene->removeItem(m_currItem);
          delete m_currItem;
        }

        m_currItem =
            new annotation::LineItem(annotation::Annotation(line, color));
        m_currItem->updateScaleFactor(m_scaleFactor);
        m_Scene->addItem(m_currItem);

        break;
      }

    } else if (event->button() == Qt::MiddleButton) {
      m_cursor.middleButton = true;
      panStartEvent(event);
      return;
    }
    return QGraphicsView::mousePressEvent(event);
  }

  void mouseMoveEvent(QMouseEvent *event) override {
    m_cursor.pos = m_PixmapItem->mapFromScene(mapToScene(event->pos()));

    if (m_cursor.leftButton) {
      switch (m_cursorMode) {
      case CursorMode::Default:
        break;
      case CursorMode::MeasureLine: {
        event->accept();

        if (auto *item = dynamic_cast<annotation::LineItem *>(m_currItem);
            item != nullptr) {
          const auto line = m_cursor.line();
          const auto distance = geometry::calcMagnitude(line.p1() - line.p2());

          item->setLine(line);
          item->setText(QString("%1 px").arg(distance, 5, 'd', 0));
        }

      } break;
      }

    } else if (m_cursor.middleButton) {
      panMoveEvent(event);
      return;
    }

    return QGraphicsView::mouseMoveEvent(event);
  }

  void mouseReleaseEvent(QMouseEvent *event) override {
    if (event->button() == Qt::LeftButton) {
      m_cursor.leftButton = false;

      switch (m_cursorMode) {
      case CursorMode::Default:
        break;
      case CursorMode::MeasureLine: {
        const auto line = m_cursor.line();

        // Save annotation
        if (m_currItem != nullptr) {
          // const auto anno = m_currItem->annotation();

          // Delete currItem
          m_Scene->removeItem(m_currItem);
          delete m_currItem;
          m_currItem = nullptr;
        }

      } break;
      }

    } else if (event->button() == Qt::MiddleButton) {
      m_cursor.middleButton = false;
      panEndEvent(event);
    }
    return QGraphicsView::mouseReleaseEvent(event);
  }

private:
  QGraphicsScene *m_Scene;
  QPixmap m_Pixmap;
  QGraphicsItem *m_PixmapItem{};
  ImageOverlay *m_overlay;

  double m_scaleFactor{1.0};
  double m_scaleFactorMin{1.0};
  bool m_resetZoomOnNext{true};

  CursorState m_cursor;
  CursorMode m_cursorMode{CursorMode::MeasureLine};
  QPoint m_lastPanPoint;
  QCursor m_lastPanCursor;

  annotation::GraphicsItemBase *m_currItem{};

  void updateTransform() {
    // Set the transformation anchor to under the mouse
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    // Update the transformation matrix
    auto transform = QTransform();
    transform.scale(m_scaleFactor, m_scaleFactor);
    setTransform(transform);

    // Update overlay zoom level (if applicable)
    m_overlay->setZoom(m_scaleFactor);
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

  bool gestureEvent(QGestureEvent *event) {
    if (QGesture *pinch = event->gesture(Qt::PinchGesture)) {
      pinchTriggered(dynamic_cast<QPinchGesture *>(pinch));
    }
    return true;
  }

  void pinchTriggered(QPinchGesture *gesture) {
    if (gesture->state() == Qt::GestureState::GestureUpdated) {
      const qreal scaleFactor = gesture->scaleFactor();
      m_scaleFactor = std::max(scaleFactor * m_scaleFactor, m_scaleFactorMin);
      updateTransform();
    }
  }
};