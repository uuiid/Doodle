//
// Created by TD on 2022/2/25.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_core/gui_template/gui_process.h>
namespace doodle {

class DOODLELIB_API image_load_task : public process_handy_tools {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  explicit image_load_task(const entt::handle& in_handle);
  ~image_load_task() override;

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update();
};

}  // namespace doodle
