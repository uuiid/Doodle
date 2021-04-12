#include <doodle_GUI/source/ScreenshotWidght/ScreenshotWidght.h>
#include <doodle_GUI/source/ScreenshotWidght/ScreenshotAction.h>
#include <corelib/Exception/Exception.h>

#include <corelib/core_Cpp.h>
#include <loggerlib/Logger.h>

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
      p_action(nullptr) {
  auto layout = new QVBoxLayout(this);

  p_butten->setText("点击截图");
  p_image->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  p_image->setAlignment(Qt::AlignCenter);

  connect(p_butten, &QPushButton::clicked,
          this, &ScreenshotWidght::createScreenshot);

  layout->addWidget(p_image, 10);
  layout->addWidget(p_butten, 1);
}

void ScreenshotWidght::createScreenshot() {
  // if (p_file_archive.expired()) return;

  // auto k_file  = p_file_archive.lock();
  // auto k_image = std::make_unique<ScreenshotArchive>(k_file);
  // auto k_cache = coreSet::getSet().getCacheRoot() / k_file->generatePath("doodle", ".png");

  // p_action        = new ScreenshotAction(this);
  // auto windowList = QGuiApplication::topLevelWindows();
  // auto winMain    = QGuiApplication::instance()->findChild<QWidget *>("mainWindows");

  // if (!boost::filesystem::exists(k_cache.parent_path())) {
  //   boost::filesystem::create_directories(k_cache.parent_path());
  // }

  // //隐藏主窗口
  // for (auto &&win : windowList) {
  //   if (win->objectName() == "mainWindowsWindow")
  //     win->hide();
  // }

  // p_action->screenShot(k_cache);

  // // 显示主窗口
  // for (auto &&win : windowList) {
  //   if (win->objectName() == "mainWindowsWindow")
  //     win->showMaximized();
  // }
  // // k_image->update(k_cache);

  // auto k_pix = QPixmap();
  // if (k_pix.load(QString::fromStdString(k_cache.generic_string())))
  //   p_image->setPixmap(k_pix.scaled(p_image->geometry().size(), Qt::KeepAspectRatio));
  // else {
  // }
}

void ScreenshotWidght::showImage() {
  // if (p_file_archive.expired()) return;

  // auto k_file  = p_file_archive.lock();
  // auto k_image = std::make_unique<ScreenshotArchive>(k_file);
  // auto k_cache = k_image->down();
  // if (!boost::filesystem::exists(k_cache)) {
  //   DOODLE_LOG_WARN("没有文件： " << k_cache.generic_string())
  //   throw FileError(k_cache.generic_string(), "没有文件");
  // }
  // auto k_pix = QPixmap();
  // if (k_pix.load(QString::fromStdString(k_cache.generic_string())))
  //   p_image->setPixmap(k_pix.scaled(p_image->geometry().size(), Qt::KeepAspectRatio));
  // else {
  //   clearImage();
  // }
}

void ScreenshotWidght::clearImage() {
  p_image->clear();
  p_image->setText(QString::fromUtf8("请截图"));
}

void ScreenshotWidght::disableButten(bool disable) {
  p_butten->setEnabled(disable);
}

DOODLE_NAMESPACE_E