//
// Created by TD on 2021/9/23.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

#include <boost/signals2.hpp>
namespace doodle {
class DOODLELIB_API edit_widgets : public process_t<edit_widgets> {
  class impl;
  std::unique_ptr<impl> p_i;

  void edit_handle();
  void add_handle();
  void clear_handle();
  void notify_file_list() const;

 public:
  edit_widgets();
  ~edit_widgets();

  constexpr static std::string_view name{"编辑"};

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(delta_type, void* data);
};
}  // namespace doodle
