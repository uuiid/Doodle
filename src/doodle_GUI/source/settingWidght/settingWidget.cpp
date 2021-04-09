//
// Created by teXiao on 2020/10/19.
//

#include <doodle_GUI/source/SettingWidght/SettingWidget.h>
#include <loggerlib/Logger.h>

#include <corelib/core_Cpp.h>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QtCore/QDir>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QFileDialog>
DOODLE_NAMESPACE_S
SettingWidget::SettingWidget(QWidget *parent)
    : QWidget(parent) {
  setWindowFlags(Qt::Window);
  auto layout = new QGridLayout(this);

  // 创建小部件
  auto &set        = coreSet::getSet();
  auto &ue_set     = Ue4Setting::Get();
  auto p_dep_label = new QLabel(tr("部门:"));
  p_dep_text       = new QComboBox();
  p_dep_text->addItems({"Executive",
                        "Light", "VFX",
                        "modle", "rig",
                        "Anm", "direct",
                        "paint"});
  p_dep_text->setCurrentText(QString::fromStdString(set.getDepartment()));

  auto p_user_label          = new QLabel(tr("用户名:"));
  auto p_cache_label         = new QLabel("缓存路径");
  auto p_user_doc_label      = new QLabel("查找到的文档路径");
  auto p_ue_path_label       = new QLabel(tr("ue4 安装路径"));
  auto p_ue_version_label    = new QLabel(tr("ue4 查找版本"));
  auto p_ue_shot_start_label = new QLabel(tr("ue4 导入镜头开始"));
  auto p_ue_shot_end_label   = new QLabel(tr("ue4 导入镜头结束"));
  auto p_cache               = new QLabel(QString::fromStdString(
      set.getCacheRoot().generic_string()));
  auto p_user_doc            = new QLabel(QString::fromStdString(set.getDoc().generic_string()));
  auto p_ue_open_path        = new QPushButton("...");
  p_ue_open_path->setToolTip(R"(这里要选择的目录是
.egstore
Engine
FeaturePacks
Samples
Templates
这些目录的上一级)");

  p_user_text     = new QLineEdit(QString::fromStdString(set.getUser()));
  p_save          = new QPushButton(tr("保存"));
  p_ue_path       = new QLineEdit(QString::fromStdString(
      ue_set.Path().generic_string()));
  p_ue_version    = new QLineEdit(QString::fromStdString(
      ue_set.Version()));
  p_ue_shot_start = new QSpinBox();
  p_ue_shot_end   = new QSpinBox();

  p_ue_shot_start->setMinimum(0);
  p_ue_shot_end->setMaximum(9999);

  // 创建布局
  // 标签
  layout->addWidget(p_dep_label, 0, 0, 1, 2);
  layout->addWidget(p_user_label, 1, 0, 1, 2);
  layout->addWidget(p_cache_label, 2, 0, 1, 2);
  layout->addWidget(p_user_doc_label, 3, 0, 1, 2);
  layout->addWidget(p_ue_path_label, 4, 0, 1, 2);
  layout->addWidget(p_ue_version_label, 5, 0, 1, 2);
  layout->addWidget(p_ue_shot_start_label, 6, 0, 1, 2);
  layout->addWidget(p_ue_shot_end_label, 7, 0, 1, 2);

  layout->addWidget(p_ue_open_path, 4, 2, 1, 1);

  layout->addWidget(p_dep_text, 0, 1, 1, 2);
  layout->addWidget(p_user_text, 1, 1, 1, 2);
  layout->addWidget(p_cache, 2, 1, 1, 2);
  layout->addWidget(p_user_doc, 3, 1, 1, 2);
  layout->addWidget(p_ue_path, 4, 1, 1, 1);
  layout->addWidget(p_ue_version, 5, 1, 1, 2);
  layout->addWidget(p_ue_shot_start, 6, 1, 1, 2);
  layout->addWidget(p_ue_shot_end, 7, 1, 1, 2);

  layout->setColumnStretch(0, 2);
  layout->setColumnStretch(1, 5);
  layout->setColumnStretch(2, 1);

  // 连接更改函数
  connect(p_save, &QPushButton::clicked,
          this, [&set]() { set.writeDoodleLocalSet(); });

  connect(p_dep_text, &QComboBox::currentTextChanged,
          this, [&set](const QString &str) { set.setDepartment(str.toStdString()); });
  connect(p_user_text, &QLineEdit::textEdited,
          this, [&set](const QString &str) { set.setUser(str.toStdString()); });
  connect(p_ue_path, &QLineEdit::textEdited,
          this, [&ue_set, this](const QString &str) {
            ue_set.setPath(str.toStdString());
            this->p_ue_version->setDisabled(!str.isEmpty());
          });
  connect(p_ue_version, &QLineEdit::textEdited, this,
          [&ue_set](const QString &str) { ue_set.setVersion(str.toStdString()); });
  connect(p_ue_shot_start, qOverload<int>(&QSpinBox::valueChanged), this,
          [&ue_set](int start) { ue_set.setShotStart(start); });
  connect(p_ue_shot_end, qOverload<int>(&QSpinBox::valueChanged), this,
          [&ue_set](int end) { ue_set.setShotEnd(end); });

  connect(p_ue_open_path, &QPushButton::clicked, this,
          [&ue_set, this]() {
            auto path = QFileDialog::getExistingDirectory(this, QString{"ue安装目录"});
            if (!path.isEmpty()) {
              this->p_ue_path->setText(path);
              ue_set.setPath(path.toStdString());
              this->p_ue_version->setDisabled(true);
            }
          });
  resize({600, 200});
}

void SettingWidget::setInit() {
  auto &ue_set = Ue4Setting::Get();
  p_dep_text->setCurrentText(QString::fromStdString(p_set_.getDepartment()));
  p_user_text->setText(QString::fromStdString(p_set_.getUser()));
  p_ue_path->setText(QString::fromStdString(ue_set.Path().generic_string()));
  p_ue_version->setText(QString::fromStdString(ue_set.Version()));
  p_ue_shot_start->setValue(ue_set.ShotStart());
  p_ue_shot_end->setValue(ue_set.ShotEnd());
}

void SettingWidget::save() {
  try {
    p_set_.writeDoodleLocalSet();
  } catch (const DoodleError &error) {
    QMessageBox::warning(this, QString::fromUtf8("注意: "), tr(error.what()));
  }
}

void SettingWidget::closeEvent(QCloseEvent *event) {
  this->save();
  QWidget::closeEvent(event);
}

DOODLE_NAMESPACE_E