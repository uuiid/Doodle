//
// Created by TD on 2022/5/30.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle {
namespace database_n {

class DOODLE_CORE_EXPORT select : public process_t<select> {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  using base_type = process_t<select>;
  class DOODLE_CORE_EXPORT arg {
   public:
    FSys::path project_path;
    bool only_ctx;
  };
  explicit select(const arg& in_arg);

  ~select() override;
  void init();
  void succeeded();
  void failed();
  void aborted();
  void update(base_type::delta_type, void* data);

 private:
  void select_db();
  void select_ctx();

  bool chick_table();
  void update();
};

}  // namespace database_n
}  // namespace doodle
