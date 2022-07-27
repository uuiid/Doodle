//
// Created by TD on 2022/5/30.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/gui_template/gui_process.h>
namespace doodle::database_n {

class DOODLE_CORE_EXPORT delete_data : public process_handy_tools {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  using base_type = process_t<delete_data>;
  explicit delete_data(const std::vector<entt::entity>& in_data);

  ~delete_data() override;
  void init();

  void aborted();
  void update();
};

}  // namespace doodle::database_n
