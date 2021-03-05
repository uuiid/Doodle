#pragma once

#include <MotionGlobal.h>

#include <boost/signals2.hpp>

#include <QtWidgets/QTreeView>
namespace doodle::motion::ui {
class TreeDirView : public QTreeView {
  Q_OBJECT
 private:
 public:
  TreeDirView(QWidget* parent = nullptr);
  ~TreeDirView();

  boost::signals2::signal<void(const FSys::path& path, const QModelIndex& index)> sig_chickItem;

 protected:
  //上下文菜单
  void
  contextMenuEvent(QContextMenuEvent* event) override;

 private:
  void createDir();
  void editDir();

  void doodleChicked(const QModelIndex& index);
};

}  // namespace doodle::motion::ui