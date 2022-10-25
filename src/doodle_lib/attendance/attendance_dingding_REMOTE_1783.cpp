//
// Created by TD on 2022/10/21.
//

#include "attendance_dingding.h"

#include <doodle_core/metadata/user.h>

#include <boost/asio.hpp>

#include <doodle_dingding/client/dingding_api.h>
#include <doodle_dingding/metadata/attendance.h>
namespace doodle {
namespace business {

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
  if (in_handle.all_of<user>())
    doodle::throw_error(doodle::error_enum::component_missing_error, fmt::format("句柄 {} 缺少用户组件", in_handle));

  ptr->user_handle = in_handle;
}
void attendance_dingding::set_range(const time_point_wrap& in_begin, const time_point_wrap& in_end) {
  ptr->begin_time = in_begin;
  ptr->end_time   = in_end;
}

///获取考勤资源
void attendance_dingding::get_resources(const dingding::attendance::attendance& get_data,
                                        const dingding::attendance::attendance& get_update_data) {
  ptr->get_day_data =get_data;
  ptr->get_update_data=get_update_data;
}

void attendance_dingding::alt_clock(const doodle::business::work_clock& alt_workclock){
  ptr->gen_rules_=alt_workclock;
}

const work_clock& attendance_dingding::work_clock_attr() const {
  if (!ptr->client) {
    ptr->ssl_context = std::move(std::make_unique<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23));
    ptr->client =
        std::move(std::make_unique<doodle::dingding::dingding_api>(g_io_context().get_executor(), *ptr->ssl_context));
  }
   /// @brief  从客户端中获取考勤资源  -> 转换为 work_cloc
  {
    ///get_data=get_data+work_weekdays-extra_holiday-extra_rest+extra_work
    ///获取初始的考勤资源
    ///要将初始的考勤资源-节假日-调休+加班+工作日规定的时间
    ///
    ///work_clock_attr=get_data()
  }


  return ptr->work_clock_attr;
}

attendance_dingding::~attendance_dingding() = default;
}  // namespace business

}  // namespace doodle





