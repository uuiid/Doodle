//
// Created by TD on 2022/10/21.
//

#include "attendance_dingding.h"

#include <doodle_core/metadata/user.h>

#include <boost/asio.hpp>

#include <doodle_dingding/client/dingding_api.h>
#include <doodle_dingding/metadata/attendance.h>
#include <doodle_dingding/metadata/access_token.h>
#include <doodle_dingding/metadata/attendance.h>
#include <doodle_dingding/metadata/department.h>
#include <doodle_dingding/metadata/request_base.h>
#include <doodle_dingding/metadata/user.h>

namespace doodle::business {

class attendance_dingding::impl {
 public:
  std::unique_ptr<doodle::dingding::dingding_api> client{};
  std::unique_ptr<boost::asio::ssl::context> ssl_context{};

  work_clock work_clock_attr{};
  work_clock gen_rules_{};
  entt::handle user_handle{};
  time_point_wrap begin_time{};
  time_point_wrap end_time{};
  dingding::attendance::attendance get_day_data{};
  dingding::attendance::attendance get_update_data{};
};

attendance_dingding::attendance_dingding() : ptr(std::make_unique<impl>()) {}

void attendance_dingding::set_user(const entt::handle& in_handle) {
  if (in_handle.all_of<doodle::dingding::user>())
    doodle::throw_error(doodle::error_enum::component_missing_error, fmt::format("句柄 {} 缺少用户组件", in_handle));

  ptr->user_handle = in_handle;
}
void attendance_dingding::set_range(const time_point_wrap& in_begin, const time_point_wrap& in_end) {
  ptr->begin_time = in_begin;
  ptr->end_time   = in_end;
}

const work_clock& attendance_dingding::work_clock_attr() const {
  if (!ptr->client) {
    ptr->ssl_context = std::move(std::make_unique<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23));
    ptr->client =
        std::move(std::make_unique<doodle::dingding::dingding_api>(g_io_context().get_executor(), *ptr->ssl_context));
  }

  auto l_user = ptr->user_handle.get<doodle::dingding::user>();
  if (l_user.phone_number.empty()) throw_error(doodle::error_enum::null_string, "用户电话号码为空");

  {  /// @brief  从客户端中获取考勤资源  -> 转换为 work_clock
  }


  return ptr->work_clock_attr;
}

attendance_dingding::~attendance_dingding() = default;
}  // namespace doodle::business