//
// Created by TD on 2021/5/14.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
class wxDataViewModel;

namespace doodle {

class ContextMenu {
  wxMenu* p_menu;
  wxWindow* p_parent;
  MetadataFactoryPtr p_metadata_flctory_ptr_;
  wxDataViewModel* p_model;

  friend Project;
  friend Episodes;
  friend Shot;
  friend Assets;
  friend AssetsFile;

  wxMenu* createMenu(const ProjectPtr& in_data);
  wxMenu* createMenu(const EpisodesPtr& in_data);
  wxMenu* createMenu(const ShotPtr& in_data);
  wxMenu* createMenu(const AssetsPtr& in_data);
  wxMenu* createMenu(const AssetsFilePtr& in_data);

  wxMenu* createMenuAfter(const MetadataPtr& in_data);

  ProjectPtr createProject();

 public:
  /**
   * 这个类是用来创建上下文菜单的， 每次重新生成，同时这个类也是和上下文结合的比较紧密的类
   *
   *  在创建后使用：
   *    Metadata* ptr->createMenu(ContextMenu* this);
   *    调用生成上下文菜单
   *
   *  这个类并不获得菜单的所有权
   *  @param in_menu 这个是创建上下文菜单需要传入的菜单指针
   */
  explicit ContextMenu(wxWindow* in_parent, wxMenu* in_menu, wxDataViewModel* in_model);
  wxMenu* createMenuAfter();
  [[nodiscard]] std::string getShotAb() const;
};

}  // namespace doodle
