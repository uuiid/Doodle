#include "ScreenshotWidght.h"

#include <core_Cpp.h>

#include <src/ScreenshotWidght/ScreenshotAction.h>

#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlayout.h>

DOODLE_NAMESPACE_S

ScreenshotWidght::ScreenshotWidght(QWidget *parent)
    : QWidget(parent),
      p_butten(new QPushButton()),
      p_image(new QLabel()),
      p_action(nullptr),
      p_file_archive() {
  p_action    = new ScreenshotAction();
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
  // if (p_file_archive.expired()) return;

  // auto k_file = p_file_archive.lock();

  // auto k_image = std::make_shared<ScreenshotArchive>(k_file);

  // k_image->update("");

  p_action->screenShot("D:/tmp/rear.png");
}

void ScreenshotWidght::showImage() {
}

DOODLE_NAMESPACE_E