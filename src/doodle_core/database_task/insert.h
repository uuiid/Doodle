//
// Created by TD on 2022/5/30.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/gui_template/gui_process.h>
namespace doodle::database_n {

class DOODLE_CORE_EXPORT insert : public process_handy_tools {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  using base_type = process_t<insert>;
  explicit insert(const std::vector<entt::entity>& in_inster);

  ~insert() override;
  [[maybe_unused]] void init();

  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update();
};

}  // namespace doodle::database_n
