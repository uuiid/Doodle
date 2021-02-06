#include "queueManagerWidget.h"

#include <corelib/filesystem/FileSystem.h>

#include <doodle_GUI/source/queueManager/view/queueListWidget.h>
#include <doodle_GUI/source/queueManager/model/queueListModel.h>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPlainTextEdit>

DOODLE_NAMESPACE_S

queueManagerWidget::queueManagerWidget(QWidget* parent)
    : QWidget(parent) {
  auto layout = new QVBoxLayout{this};
  // auto k_queueManager = new queueListWidget{};
  // auto model          = new queueListModel{};
  p_plain = new QPlainTextEdit{};

  p_plain->setReadOnly(true);
  p_plain->setMaximumBlockCount(1000);

  auto highlightBlock = new QueueSyntaxHighlighter{p_plain->document()};
  // connect(p_plain, &QPlainTextEdit::textChanged, highlightBlock, &QueueSyntaxHighlighter::rehighlight);
  DfileSyntem::get().filelog.connect([=](const std::string& k_msg) { this->addTest(QString::fromStdString(k_msg)); });
  DfileSyntem::get().fileStreamLog.connect([=](const std::string& k_msg) { this->addTest(QString::fromStdString(k_msg)); });
  connect(this, &queueManagerWidget::addTest,
          p_plain, &QPlainTextEdit::appendPlainText, Qt::QueuedConnection);

  setMaximumHeight(150);
  // k_queueManager->setModel(model);
  // layout->addWidget(k_queueManager);
  layout->addWidget(p_plain);
}

DOODLE_NAMESPACE_E