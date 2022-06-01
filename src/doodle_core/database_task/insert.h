//
// Created by TD on 2022/5/30.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle::database_n {

class DOODLE_CORE_EXPORT insert : public process_t<insert> {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  using base_type = process_t<insert>;
  explicit insert(const std::vector<entt::entity>& in_inster);

  ~insert() override;
  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(base_type::delta_type, void* data);
};

}  // namespace doodle::database_n
