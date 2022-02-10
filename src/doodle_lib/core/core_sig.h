//
// Created by TD on 2022/1/25.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <boost/signals2.hpp>
namespace doodle {

class DOODLELIB_API core_sig {
 public:
  boost::signals2::signal<void(const FSys::path&)> project_begin_open;
  boost::signals2::signal<void(const entt::handle&, const project&)> project_end_open;

  boost::signals2::signal<void(const std::vector<entt::handle>&)> filter_handle;
  boost::signals2::signal<void(const entt::handle&)> select_handle;
  boost::signals2::signal<void(const std::vector<entt::handle>&)> select_handles;

  boost::signals2::signal<void()> save;
};
}  // namespace doodle
