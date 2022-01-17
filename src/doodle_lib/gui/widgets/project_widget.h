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
class DOODLELIB_API project_widget : public process_t<project_widget> {
 public:
  project_widget();
  ~project_widget() override;

  constexpr static std::string_view name{"项目"};

  bool show{true};

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(delta_type, void* data);
  void render();
  entt::handle p_c;

  boost::signals2::signal<void(const entt::entity&)> select_change;
};
}  // namespace doodle
