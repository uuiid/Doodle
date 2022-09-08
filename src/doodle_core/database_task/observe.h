//
// Created by TD on 2022/5/30.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle {
namespace database_n {

class DOODLE_CORE_API observe {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  explicit observe(const std::vector<entt::entity>& in_data);

  virtual ~observe();
  void init();
  void succeeded();
  void failed();
  void aborted();
  void update();
};

}  // namespace database_n
}  // namespace doodle
