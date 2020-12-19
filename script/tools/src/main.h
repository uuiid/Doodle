/*
 * @Author: your name
 * @Date: 2020-12-01 15:47:51
 * @LastEditTime: 2020-12-01 18:05:17
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \tools\src\main.h
 */
//
// Created by teXiao on 2020/12/1.
//
#include <QtCore/QObject>
#include <QtWidgets/QWidget>
#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>
#include <QtCore/QRunnable>

class QLineEdit;
class QPushButton;

namespace doodle {
class mayaAbcExport : public QWidget {
  Q_OBJECT
 public:
  mayaAbcExport(QWidget *parent = nullptr);

 private Q_SLOTS:
  void openDialogGetDir();
  void setDirString(const QString &text);
  void exportFbxFile();

 private:
  QLineEdit *p_lineEdit_;
  QPushButton *p_get_path_button_;
  QPushButton *p_export_button_;

  QDir p_dir;
  QTemporaryFile p_pyPath_;
};
class runExport : public QRunnable {
 public:
  runExport(const QString &command);
  void run() override;

 private:
  QString p_command_;
};
}  // namespace doodle