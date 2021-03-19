#include <doodle_GUI/doodle_global.h>

#include <QtWidgets/qpushbutton.h>
#include <boost/signals2.hpp>

DOODLE_NAMESPACE_S

class DragPushBUtton : public QPushButton {
  Q_OBJECT;

 private:
 public:
  DragPushBUtton(QWidget* parent = nullptr);

  boost::signals2::signal<void(const std::vector<pathPtr>&)> handleFileFunction;

 protected:
  //上下文菜单
  // void contextMenuEvent(QContextMenuEvent* event) override;

  //拖拽函数
  void dragMoveEvent(QDragMoveEvent* event) override;
  //拖拽函数
  void dragLeaveEvent(QDragLeaveEvent* event) override;
  //拖拽函数
  void dragEnterEvent(QDragEnterEvent* event) override;
  //拖拽函数
  void dropEvent(QDropEvent* event) override;
  void enableBorder(const bool& enabled);
};

DOODLE_NAMESPACE_E