//
// Created by TD on 2022/2/7.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <entt/entt.hpp>

namespace doodle {

class DOODLELIB_API project_edit : public process_t<project_edit> {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  project_edit();
  ~project_edit() override;
  constexpr static std::string_view name{"项目设置"};
  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const delta_type&, void* data);
};

}  // namespace doodle
