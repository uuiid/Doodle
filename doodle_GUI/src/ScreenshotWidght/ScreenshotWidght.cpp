#include "ScreenshotWidght.h"

#include <core_Cpp.h>
#include <Logger.h>
#include <src/ScreenshotWidght/ScreenshotAction.h>

#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlayout.h>

#include <QtWidgets/qapplication.h>
#include <QtGui/qwindow.h>
DOODLE_NAMESPACE_S

ScreenshotWidght::ScreenshotWidght(QWidget *parent)
    : QWidget(parent),
      p_butten(new QPushButton()),
      p_image(new QLabel()),
      p_action(nullptr),
      p_file_archive() {
  auto layout = new QVBoxLayout(this);

  p_butten->setText("点击截图");
  p_image->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  p_image->setAlignment(Qt::AlignCenter);

  connect(p_butten, &QPushButton::clicked,
          this, &ScreenshotWidght::createScreenshot);

  layout->addWidget(p_image, 10);
  layout->addWidget(p_butten, 1);
}

void ScreenshotWidght::createScreenshot() {
  if (p_file_archive.expired()) return;

  auto k_file  = p_file_archive.lock();
  auto k_image = std::make_unique<ScreenshotArchive>(k_file);
  auto k_cache = coreSet::getSet().getCacheRoot() / k_file->generatePath("doodle", ".png");

  p_action        = new ScreenshotAction(this);
  auto windowList = QGuiApplication::topLevelWindows();
  auto winMain    = QGuiApplication::instance()->findChild<QWidget *>("mainWindows");

  //隐藏主窗口
  for (auto &&win : windowList) {
    if (win->objectName() == "mainWindowsWindow")
      win->hide();
  }

  p_action->screenShot(k_cache);

  // 显示主窗口
  for (auto &&win : windowList) {
    if (win->objectName() == "mainWindowsWindow")
      win->showMaximized();
  }
  k_image->update(k_cache);
}

void ScreenshotWidght::showImage() {
  if (p_file_archive.expired()) return;

  auto k_file  = p_file_archive.lock();
  auto k_image = std::make_unique<ScreenshotArchive>(k_file);
  auto k_cache = k_image->down();

  auto k_pix = QPixmap();
  if (k_pix.load(QString::fromStdString(k_cache.generic_string())))
    p_image->setPixmap(k_pix);
  else {
  }
}

void ScreenshotWidght::disableButten(bool disable) {
  p_butten->setEnabled(disable);
}

DOODLE_NAMESPACE_E