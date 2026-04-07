#pragma once

#include "doodle_core/doodle_core_fwd.h"

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/rational.hpp>

#include <string>


namespace doodle {
class progress_data {
  using reational_t = boost::rational<std::int64_t>;
  reational_t progress_rational_{0, 1};

  // 总共分为多少步
  reational_t total_steps_{1};
  // 当前一共多少步
  reational_t current_steps_{1};

  void update_progress(std::int32_t in_progress);
  // 百分比发送事件标识
  reational_t last_emitted_progress_{0, 100};
  uuid uuid_id_;
  std::string namespace_{};
  std::string event_name_{};

 public:
  explicit progress_data(uuid in_uuid, std::string in_event_name, std::string in_namespace = "/events")
      : uuid_id_(in_uuid), namespace_(std::move(in_namespace)), event_name_(std::move(in_event_name)) {};
  void set_total_steps(std::int32_t in_total_steps);
  std::int32_t get_total_steps() const;

  void set_current_steps(std::int32_t in_current_steps);
  std::int32_t get_current_steps() const;

  // 重载 ++ 运算符，每调用一次表示完成了一步
  progress_data& operator++();
};
using progress_data_ptr = std::shared_ptr<progress_data>;

}  // namespace doodle