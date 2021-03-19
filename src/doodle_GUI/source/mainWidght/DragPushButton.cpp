#include <doodle_GUI/source/mainWidght/DragPushBUtton.h>
#include <QtGui/QDragMoveEvent>
#include <QtCore/QMimeData>
#include <QtWidgets/qmessagebox.h>

DOODLE_NAMESPACE_S
DragPushBUtton::DragPushBUtton(QWidget* parent)
    : QPushButton(parent),
      handleFileFunction() {
  setAcceptDrops(true);
}

void DragPushBUtton::dragMoveEvent(QDragMoveEvent* event) {
}

void DragPushBUtton::dragLeaveEvent(QDragLeaveEvent* event) {
  enableBorder(false);
}

void DragPushBUtton::dragEnterEvent(QDragEnterEvent* event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
    enableBorder(true);
  } else
    event->ignore();
}

void DragPushBUtton::dropEvent(QDropEvent* event) {
  QPushButton::dropEvent(event);
  if (!event->mimeData()->hasUrls())
    return enableBorder(false);
  auto k_urls = event->mimeData()->urls();
  auto k_list = std::vector<pathPtr>{};
  for (auto&& item : k_urls) {
    if (item.isLocalFile())
      k_list.emplace_back(std::make_shared<FSys::path>(item.toLocalFile().toStdWString()));
  }
  if (k_list.empty()) {
    QMessageBox::warning(this, tr("警告"), tr("无法找到拖入的文件信息"));
  } else {
    try {
      this->handleFileFunction(k_list);
    } catch (const std::runtime_error& err) {
      QMessageBox::warning(this, tr("警告"), tr("处理文件失败"));
    }
  }

  return enableBorder(false);
}

void DragPushBUtton::enableBorder(const bool& enabled) {
  if (enabled)
    setStyleSheet("border:3px solid #165E23");
  else
    setStyleSheet("");
}

DOODLE_NAMESPACE_E