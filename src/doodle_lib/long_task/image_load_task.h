//
// Created by TD on 2022/2/25.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

class DOODLELIB_API image_load_task : public process_t<image_load_task> {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  explicit image_load_task(const entt::handle& in_handle);
  ~image_load_task();

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const delta_type&, void* data);
};
}  // namespace doodle
