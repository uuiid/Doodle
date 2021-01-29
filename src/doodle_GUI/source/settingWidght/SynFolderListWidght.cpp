#include "SynFolderListWidght.h"

#include <QtWidgets/QListWidget>

DOODLE_NAMESPACE_S

SynFolderListWidght::SynFolderListWidght(QWidget *parent)
    : QWidget(parent),
      p_local_path_list(new QListWidget()),
      p_server_path_list(new QListWidget()) {
  auto layout = new QHBoxLayout(this);


  layout->addWidget(p_local_path_list);
  layout->addWidget(p_server_path_list);
}
DOODLE_NAMESPACE_E
