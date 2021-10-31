//
// Created by TD on 2021/9/16.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

#include <boost/signals2.hpp>
namespace doodle {
/**
 * @brief 项目窗口
 * @image html doodle_project_windows.jpg 项目窗口
 * 这个窗口显示了项目的各种参数
 *
 */
class DOODLELIB_API project_widget : public metadata_widget {
  template <typename... arg>
  class add_comm_guard {
   private:
    entt::handle p_curr;

   public:
    add_comm_guard() : p_curr(){};

    add_comm_guard& operator=(entt::handle in) {
      if (p_curr) {
        p_curr.remove<arg...>();
      }
      p_curr = in;
      p_curr.get_or_emplace<arg...>();
    };

    operator entt::entity() const {
      return p_curr;
    }
  };

 public:
  project_widget();
  void frame_render() override;

  entt::entity p_current_select;

  boost::signals2::signal<void(const entt::entity&)> select_change;
};
}  // namespace doodle
