#pragma once

#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/metadata/detail/user_ref.h"
#include "doodle_core/metadata/metadata.h"
#include "doodle_core/metadata/time_point_wrap.h"

#include <chrono>
#include <date/date.h>
#include <entt/entity/fwd.hpp>
#include <string>

namespace doodle {
/**
 * @brief 任务类
 *
 */
class DOODLE_CORE_API work_task_info {
 public:
  using time_point_type = chrono::sys_time<chrono::hours>;
  work_task_info()      = default;

  explicit work_task_info(
      entt::handle in_user_ref, time_point_type in_time, std::string in_task_name, std::string in_region,
      std::string in_abstract
  )
      : user_ref(in_user_ref),
        time(in_time),
        task_name(std::move(in_task_name)),
        region(std::move(in_region)),
        abstract(std::move(in_abstract)){};

  /// 任务人引用
  user_ref user_ref{};
  /// 时间
  time_point_type time{};
  /// 名称
  std::string task_name{};
  /// 地点
  std::string region{};
  /// 概述
  std::string abstract{};

 private:
  friend void DOODLE_CORE_API to_json(nlohmann::json& j, const work_task_info& p);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& j, work_task_info& p);
};
}  // namespace doodle