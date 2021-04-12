#include <doodle_GUI/source/ScreenshotWidght/ScreenshotAction.h>
#include <corelib/Exception/Exception.h>

#include <QtWidgets/qdesktopwidget.h>
#include <QtWidgets/qapplication.h>

#include <QtGui/qscreen.h>
#include <QtGui/qevent.h>
#include <QtGui/qpainter.h>

#include <boost/filesystem.hpp>

DOODLE_NAMESPACE_S

ScreenshotAction::ScreenshotAction(QWidget *parent)
    : QDialog(parent),
      p_isDrawing_b(false),
      p_screen(nullptr),
      p_start_pos(),
      p_end_pos(),
      p_mask(),
      p_save_path("C:/doodle.png") {
  setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Window);
  setMouseTracking(true);
}

void ScreenshotAction::screenShot(const FSys::path &save_path) {
  p_save_path = save_path.generic_string();
  setStyleSheet(R"(background-color:black;)");
  setWindowOpacity(0.8);

  p_screen = QGuiApplication::screenAt(QCursor::pos());
  if (!p_screen) {
    p_screen = QGuiApplication::primaryScreen();
  }
  if (!p_screen) {
    throw nullptr_error("没有找到屏幕");
  }

  setGeometry(p_screen->geometry());
  setCursor(Qt::CrossCursor);

  p_mask = QBitmap(p_screen->geometry().size());
  p_mask.fill(Qt::color1);
  show();
  exec();
}

void ScreenshotAction::paintEvent(QPaintEvent *event) {
  if (p_isDrawing_b) {
    auto k_pix     = p_mask.copy();  //创建绘制位图
    auto k_painter = QPainter(&k_pix);
    auto k_pen     = QPen();
    k_pen.setStyle(Qt::NoPen);
    k_painter.setPen(k_pen);
    // 此处是为零小部件不可见
    auto k_bush = QBrush(Qt::color0);
    k_painter.setBrush(k_bush);
    k_painter.drawRect(QRect{p_start_pos, p_end_pos});

    setMask(QBitmap(k_pix));
  }
}

void ScreenshotAction::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    p_start_pos   = event->pos();
    p_end_pos     = event->pos();
    p_isDrawing_b = true;
  }
}

void ScreenshotAction::mouseMoveEvent(QMouseEvent *event) {
  if (p_isDrawing_b) {
    p_end_pos = event->pos();
    update();
  } /* else {
    auto k_screen = QGuiApplication::screenAt(event->globalPos());
    if (!k_screen) {
      k_screen = QGuiApplication::primaryScreen();
    }
    if (!k_screen) {
      throw nullptr_error("没有找到屏幕");
    }

    if (k_screen != p_screen) {
      // 第二个屏幕和第一个屏幕大小不一定一样
      p_screen = k_screen;
      move(event->globalPos());
      setGeometry(p_screen->geometry());
    }
  } */
}

void ScreenshotAction::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    p_end_pos = event->pos();

    auto pixmap = p_screen->grabWindow(0,
                                       p_start_pos.x(), p_start_pos.y(),
                                       p_end_pos.x() - p_start_pos.x(), p_end_pos.y() - p_start_pos.y());
    pixmap.save(QString::fromStdString(p_save_path));
    close();
  }
}

DOODLE_NAMESPACE_E