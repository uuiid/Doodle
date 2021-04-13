//
// Created by teXiao on 2020/10/19.
//

#include <doodle_GUI/source/SettingWidght/SettingWidget.h>
#include <doodle_GUI/source/SettingWidght/Model/ProjectModel.h>

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
#include <QtWidgets/qinputdialog.h>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QErrorMessage>

DOODLE_NAMESPACE_S

static SettingWidget *doodle_SettingWidget = nullptr;

SettingWidget *SettingWidget::Get() {
  if (doodle_SettingWidget == nullptr) {
    auto topWindwos = qApp->topLevelWidgets();
    for (auto w : topWindwos) {
      if (QMainWindow *mainW = qobject_cast<QMainWindow *>(w))
        doodle_SettingWidget = new SettingWidget{mainW};
    }
    if (doodle_SettingWidget == nullptr) {
      doodle_SettingWidget = new SettingWidget{};
    }
  }
  return doodle_SettingWidget;
}

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
  auto p_project_list_lable  = new QLabel{"项目列表"};
  auto p_project_add         = new QPushButton("添加");
  auto p_project_delete      = new QPushButton("删除");
  p_project_list             = new QComboBox();
  p_project_model            = new ProjectModel{this};
  p_project_list->setModel(p_project_model);

  // p_project_list->setItemDelegate(nullptr);

  p_ue_open_path->setToolTip(R"(这里要选择的目录是
.egstore
Engine
FeaturePacks
Samples
Templates
这些目录的上一级)");
  p_project_list->setModel(nullptr);
  p_user_text     = new QLineEdit(QString::fromStdString(set.getUser()));
  p_save          = new QPushButton(tr("保存(测试数据并保存)"));
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

  //第一列
  layout->addWidget(p_dep_label, 0, 0, 1, 1);
  layout->addWidget(p_user_label, 1, 0, 1, 1);
  layout->addWidget(p_cache_label, 2, 0, 1, 1);
  layout->addWidget(p_user_doc_label, 3, 0, 1, 1);
  layout->addWidget(p_project_list_lable, 4, 0, 1, 1);
  layout->addWidget(p_ue_path_label, 5, 0, 1, 1);
  layout->addWidget(p_ue_version_label, 6, 0, 1, 1);
  layout->addWidget(p_ue_shot_start_label, 7, 0, 1, 1);
  layout->addWidget(p_ue_shot_end_label, 8, 0, 1, 1);

  //第二列开始
  layout->addWidget(p_dep_text, 0, 1, 1, 4);
  layout->addWidget(p_user_text, 1, 1, 1, 4);
  layout->addWidget(p_cache, 2, 1, 1, 4);
  layout->addWidget(p_user_doc, 3, 1, 1, 4);
  layout->addWidget(p_ue_version, 6, 1, 1, 4);
  layout->addWidget(p_ue_shot_start, 7, 1, 1, 4);
  layout->addWidget(p_ue_shot_end, 8, 1, 1, 4);
  layout->addWidget(p_save, 9, 0, 1, 4);
  // 这两个占据位置不是末尾
  layout->addWidget(p_project_list, 4, 1, 1, 1);
  layout->addWidget(p_ue_path, 5, 1, 1, 2);

  //第三列开始
  layout->addWidget(p_project_add, 4, 2, 1, 1);
  layout->addWidget(p_ue_open_path, 5, 2, 1, 1);

  // 第四列开始
  layout->addWidget(p_project_delete, 4, 3, 1, 1);

  layout->setColumnStretch(0, 2);
  layout->setColumnStretch(1, 5);
  layout->setColumnStretch(2, 1);
  layout->setColumnStretch(3, 1);

  // 连接更改函数
  connect(p_save, &QPushButton::clicked,
          this, [&set, &ue_set, this]() {
            try {
              set.writeDoodleLocalSet();
            } catch (const std::exception &error) {
              QMessageBox::warning(this, QString::fromUtf8("注意: "), tr(error.what()));
            }
            p_ue_path->setText(QString::fromStdString(ue_set.Path().generic_string()));
          });

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
  connect(p_project_add, &QPushButton::clicked,
          this, &SettingWidget::addPriject);
  connect(p_project_list, qOverload<int>(&QComboBox::currentIndexChanged),
          this, &SettingWidget::setProject);
  connect(p_project_delete, &QPushButton::clicked,
          this, &SettingWidget::deleteProject);
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

  if (ue_set.hasPath())
    p_ue_version->setDisabled(true);
  p_project_list->setCurrentIndex(p_set_.getProjectIndex());
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

void SettingWidget::setProject(int index) {
  if (index >= 0) {
    auto k_prj = p_project_list->itemData(index, Qt::UserRole);
    try {
      p_set_.setProject_(k_prj.value<Project *>());
    } catch (const DoodleError &error) {
      QMessageBox::warning(this, QString::fromUtf8("注意: "), tr(error.what()));
    }
  }
}

void SettingWidget::addPriject() {
  auto path = QFileDialog::getExistingDirectory(this, QString{"项目目录"});
  auto name = QInputDialog::getText(this, tr("项目"), tr("名称: "));
  if (!path.isEmpty() && !name.isEmpty()) {
    auto prj = std::make_shared<Project>(path.toStdString(), name.toStdString());
    this->p_set_.installProject(prj);
    p_project_model->init(this->p_set_.getAllProjects());
  } else {
    if (path.isEmpty())
      QMessageBox::warning(this, QString::fromUtf8("注意: "), tr("没有找到路径, 无法添加"));
    else if (name.isEmpty())
      QMessageBox::warning(this, QString::fromUtf8("注意: "), tr("没有名称, 无法添加"));
    else
      QMessageBox::warning(this, QString::fromUtf8("注意: "), tr("未知原因, 无法添加"));
  }
  p_project_list->setCurrentIndex(p_set_.getProjectIndex());
}

void SettingWidget::deleteProject() {
  p_project_model->removeRow(p_project_list->currentIndex());
}

DOODLE_NAMESPACE_E