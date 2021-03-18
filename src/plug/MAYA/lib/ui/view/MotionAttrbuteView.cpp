#include <lib/ui/view/MotionAttrbuteView.h>

#include <lib/kernel/PlayerMotion.h>

#include <QtWidgets/qlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qtextedit.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qplaintextedit.h>

#include <QtGui/qimage.h>
#include <QtWidgets/qmessagebox.h>
// #include <QtMultimedia/qmediaplayer.h>

// #include <QtCore/qdebug.h>

namespace doodle::motion::ui {

MotionAttrbuteView::MotionAttrbuteView(QWidget* parent)
    : QWidget(parent),
      p_MotionFile(),
      p_MotionPlayer(std::make_shared<kernel::PlayerMotion>()),
      p_image(new QLabel()),
      p_user_label(new QLabel(tr("匿名"))),
      p_info_text(new QPlainTextEdit()),
      p_tiles_text(new QLineEdit(tr("无"))),
      p_tiles_connection(),
      p_info_connection(),
      p_play_connection() {
  auto layout = new QGridLayout(this);

  auto k_tiles_label = new QLabel(tr("名称: "));
  auto k_user_label  = new QLabel(tr("制作人: "));
  auto k_info_label  = new QLabel(tr("信息: "));

  //显示视频
  layout->addWidget(p_image, 0, 0, 1, 2);
  //显示标题
  layout->addWidget(k_tiles_label, 1, 0, 1, 1);
  layout->addWidget(p_tiles_text, 1, 1, 1, 1);
  //显示制作人
  layout->addWidget(k_user_label, 2, 0, 1, 1);
  layout->addWidget(p_user_label, 2, 1, 1, 1);
  //显示注释
  layout->addWidget(k_info_label, 3, 0, 1, 1);
  layout->addWidget(p_info_text, 4, 0, 1, 2);

  //设置显示图片缩放政策
  p_image->setBackgroundRole(QPalette::Base);
  p_image->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  // p_image->setScaledContents(true);

  p_MotionPlayer->fileImage.connect(
      [this](QImage& image) {
        auto k_ = image.scaled(this->p_image->size(), Qt::KeepAspectRatio);
        this->doodleSetImage(QPixmap::fromImage(k_));
      });
  connect(this, &MotionAttrbuteView::doodleSetImage,
          p_image, &QLabel::setPixmap, Qt::QueuedConnection);
  layout->setRowStretch(0, 10);
  layout->setRowStretch(1, 1);
  layout->setRowStretch(2, 1);
  layout->setRowStretch(3, 1);
  layout->setRowStretch(4, 10);

  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(1, 4);
}

void MotionAttrbuteView::setMotionFile(const kernel::MotionFilePtr& data) {
  p_MotionFile = data;
  this->p_MotionPlayer->setFile(p_MotionFile->VideoFile());
  this->p_tiles_text->setText(QString::fromStdString(p_MotionFile->Title()));
  this->p_user_label->setText(QString::fromStdString(p_MotionFile->UserName()));
  this->p_info_text->setPlainText(QString::fromStdString(p_MotionFile->Info()));

  //连接一些槽
  p_tiles_connection = this->connect(
      this->p_tiles_text, &QLineEdit::textEdited,
      [this](const QString& str) {
        if (str.length() > 16) {
          QMessageBox::warning(this, tr("警告"), tr("题目名称太长"));
        } else {
          this->p_MotionFile->setTitle(str.toStdString());
        }
      });
  p_info_connection = this->connect(
      this->p_info_text, &QPlainTextEdit::textChanged,
      [this] {
        auto str = this->p_info_text->toPlainText();
        if (str.length() > 2048) {
          QMessageBox::warning(this, tr("警告"), tr("备注太长了"));
        } else {
          this->p_MotionFile->setInfo(str.toStdString());
        }
      });

  p_play_connection = p_MotionFile->dataBeginChanged.connect(
      [this](const kernel::MotionFile*, kernel::MotionFile::InsideData) {
        this->p_MotionPlayer->stop_Player();
      });
}

void MotionAttrbuteView::doodleClear() {
  this->p_tiles_text->disconnect(p_tiles_connection);
  this->p_info_text->disconnect(p_info_connection);
  p_play_connection.disconnect();

  this->p_MotionPlayer->stop_Player();
  this->p_image->clear();
  this->p_tiles_text->clear();
  this->p_user_label->clear();
  this->p_info_text->clear();
}

}  // namespace doodle::motion::ui