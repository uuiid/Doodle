//
// Created by TD on 2022/5/30.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle {
namespace database_n {

class DOODLE_CORE_EXPORT observe : public process_t<observe> {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  using base_type = process_t<observe>;
  explicit observe(const std::vector<entt::entity>& in_data);

  ~observe() override;
  void init();
  void succeeded();
  void failed();
  void aborted();
  void update(base_type::delta_type, void* data);
};

}  // namespace database_n
}  // namespace doodle

