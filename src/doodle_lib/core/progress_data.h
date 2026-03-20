#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/rational.hpp>

namespace doodle {
class progress_data {
  using reational_t = boost::rational<std::int64_t>;
  reational_t progress_rational_{0, 1};

  // 总共分为多少步
  reational_t total_steps_{1};
  // 当前一共多少步
  reational_t current_steps_{0};

  void update_progress(std::int32_t in_progress);

 public:
  progress_data() = default;

  void set_total_steps(std::int32_t in_total_steps);
  std::int32_t get_total_steps() const;

  void set_current_steps(std::int32_t in_current_steps);
  std::int32_t get_current_steps() const;

  // 重载 ++ 运算符，每调用一次表示完成了一步
  progress_data& operator++();
};
using progress_data_ptr = std::shared_ptr<progress_data>;

}  // namespace doodle