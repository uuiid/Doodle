#pragma once

#include <doodle_global.h>
#include <QtWidgets/QWidget>

class QListWidget;

DOODLE_NAMESPACE_S
class SynFolderListWidght : public QWidget {
  Q_OBJECT
 public:
  SynFolderListWidght(QWidget* parent = nullptr);

 private:
  QListWidget* p_local_path_list;
  QListWidget* p_server_path_list;
};

DOODLE_NAMESPACE_E