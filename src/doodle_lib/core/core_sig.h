//
// Created by TD on 2022/1/25.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <boost/signals2.hpp>
namespace doodle {

class DOODLELIB_API core_sig {
 public:
  boost::signals2::signal<void(const FSys::path&)> begin_open;
  boost::signals2::signal<void(const entt::handle&, const project&)> end_open;
};
}  // namespace doodle
