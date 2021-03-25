#pragma once

#include <doodle_GUI/doodle_global.h>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressDialog>

namespace doodle {
class MessageAndProgress : public QObject {
  Q_OBJECT
  std::string p_message;
  QProgressDialog* p_progress_dialog;
  void createMessageDialog();
 Q_SIGNALS:

  void progress(int value);
  void showMessage();

 public:
  MessageAndProgress(QWidget* parent = nullptr);

  template <typename T>
  void createProgress(std::shared_ptr<T> value);
};

template <typename T>
void MessageAndProgress::createProgress(std::shared_ptr<T> value) {
  //连接进度
  value->progress.connect([this](int i) {
    this->progress(i);
  });
  //连接消息
  value->messagResult.connect([this](const std::string& message) {
    this->p_message.append(message);
  });
  //连接完成信号
  value->finished.connect([this] {
    this->showMessage();
  });

}
}  // namespace doodle