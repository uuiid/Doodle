#include <doodle_GUI/source/mainWidght/DragPushBUtton.h>
#include <QtGui/QDragMoveEvent>
#include <QtCore/QMimeData>
DOODLE_NAMESPACE_S
DragPushBUtton::DragPushBUtton(QWidget* parent) : QPushButton(parent) {
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
  if (!event->mimeData()->hasUrls()) return enableBorder(false);
  if (event->mimeData()->urls().size() != 1) return enableBorder(false);
}

void DragPushBUtton::enableBorder(const bool& enabled) {
  if (enabled)
    setStyleSheet("border:3px solid #165E23");
  else
    setStyleSheet("");
}

DOODLE_NAMESPACE_E