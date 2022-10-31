//
// Created by TD on 2022/10/21.
//

#include "attendance_dingding.h"

#include <doodle_core/metadata/user.h>

#include <doodle_dingding/client/dingding_api.h>
#include <doodle_dingding/metadata/access_token.h>
#include <doodle_dingding/metadata/attendance.h>
#include <doodle_dingding/metadata/department.h>
#include <doodle_dingding/metadata/request_base.h>

#include <boost/asio.hpp>

namespace doodle::business {

class attendance_dingding::impl {
 public:
  doodle::dingding_api_ptr client{};

  work_clock work_clock_attr{};
  entt::handle user_handle{};
  time_point_wrap begin_time{};
  time_point_wrap end_time{};
  std::string user_id{};
  dingding::attendance::attendance get_day_data{};
  dingding::attendance::attendance get_update_data{};
};

attendance_dingding::attendance_dingding() : ptr(std::make_unique<impl>()) {}

void attendance_dingding::set_user(const entt::handle& in_handle) {
  if (!in_handle.all_of<doodle::dingding::user>())
    doodle::throw_error(doodle::error_enum::component_missing_error, fmt::format("句柄 {} 缺少用户组件", in_handle));

  ptr->user_handle = in_handle;
}
void attendance_dingding::set_range(const time_point_wrap& in_begin, const time_point_wrap& in_end) {
  ptr->begin_time = in_begin;
  ptr->end_time   = in_end;
}

const work_clock& attendance_dingding::work_clock_attr() const {
  if (!ptr->client) {
    ptr->client = g_reg()->ctx().at<dingding_api_ptr>();
  }

  auto l_user = ptr->user_handle.get<doodle::dingding::user>();
  if (l_user.phone_number.empty()) throw_error(doodle::error_enum::null_string, "用户电话号码为空");

  {  /// @brief  从客户端中获取考勤资源  -> 转换为 work_clock
  }

  return ptr->work_clock_attr;
}
void attendance_dingding::async_run(const detail::attendance_interface::call_type_ptr& in_call_type_ptr) {
  if (!ptr->client) {
    ptr->client = g_reg()->ctx().at<dingding_api_ptr>();
  }

  auto l_user = ptr->user_handle.get<doodle::dingding::user>();
  if (l_user.user_id.empty() && l_user.phone_number.empty()) (*in_call_type_ptr)({error_enum::null_string}, {});
  if (l_user.user_id.empty()) {
    ptr->client->async_find_mobile_user(
        l_user.phone_number,
        [this](const boost::system::error_code& in_code, const dingding::user_dd& in_user_dd) {
          auto& l_user = ptr->user_handle.get<doodle::dingding::user>();
          database::save(ptr->user_handle);
          l_user.user_id = in_user_dd.userid;
          ptr->user_id   = in_user_dd.userid;
          this->get_work_time();
        }
    );

  } else {
    ptr->user_id = l_user.user_id;
    get_work_time();
  }
}
void attendance_dingding::get_work_time() {
  ptr->client->async_get_user_updatedata_attendance_list(
      ptr->begin_time, ptr->end_time, ptr->user_id,
      [this](const boost::system::error_code& in_code, const std::vector<dingding::attendance::attendance>& in_list) {
        for (const auto& item : in_list) {
          item.add_clock_data(ptr->work_clock_attr);
        }
      }
  );
}

attendance_dingding::~attendance_dingding() = default;
}  // namespace doodle::business