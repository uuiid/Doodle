#include <doodle_GUI/source/toolkit/MessageAndProgress.h>

namespace doodle {
void MessageAndProgress::createMessageDialog() {
  QMessageBox::information(
      dynamic_cast<QWidget*>(this->parent()),
      tr("结果:"),
      QString::fromStdString(p_message));
}

MessageAndProgress::MessageAndProgress(QWidget* parent)
    : QObject(parent),
      p_message(),
      p_progress_dialog(new QProgressDialog(parent)) {
  connect(this, &MessageAndProgress::progress,
          p_progress_dialog, &QProgressDialog::setValue,
          Qt::QueuedConnection);
  connect(this, &MessageAndProgress::showMessage,
          this, &MessageAndProgress::createMessageDialog,
          Qt::QueuedConnection);

  p_progress_dialog->setMinimum(0);
  p_progress_dialog->setMaximum(100);
}



}  // namespace doodle