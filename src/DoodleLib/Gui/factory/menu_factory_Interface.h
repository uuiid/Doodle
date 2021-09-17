//
// Created by TD on 2021/9/2.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
/**
 * @brief 创建各种菜单的辅助类
 *
 */
class DOODLELIB_API menu_factory_interface {
 public:
  virtual void create_menu(const ProjectPtr& in_ptr)    = 0;
  virtual void create_menu(const AssetsPtr& in_ptr)     = 0;
  virtual void create_menu(const EpisodesPtr& in_ptr)   = 0;
  virtual void create_menu(const ShotPtr& in_ptr)       = 0;
  virtual void create_menu(const AssetsFilePtr& in_ptr) = 0;
  virtual void create_menu(const season_ptr& in_ptr)    = 0;
};

}  // namespace doodle
