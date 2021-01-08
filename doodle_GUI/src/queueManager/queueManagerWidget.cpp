#include "queueManagerWidget.h"

#include <src/queueManager/view/queueListWidget.h>
// #include <src/queueManager/view/queueInfoWidget.h>
#include <src/queueManager/model/queueListModel.h>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPlainTextEdit>

DOODLE_NAMESPACE_S

queueManagerWidget::queueManagerWidget(QWidget *parent)
    : QWidget(parent) {
  auto layout         = new QVBoxLayout{this};
  auto k_queueManager = new queueListWidget{};
  auto model          = new queueListModel{};
  auto plain          = new QPlainTextEdit{};

  plain->setReadOnly(true);
  plain->setMaximumBlockCount(1000);

  k_queueManager->setModel(model);
  layout->addWidget(k_queueManager);
  layout->addWidget(plain);
}

DOODLE_NAMESPACE_E