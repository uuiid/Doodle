//
// Created by TD on 2022/10/21.
//

#include "attendance_dingding.h"

#include "doodle_core/exception/exception.h"
#include "doodle_core/logger/logger.h"
#include <doodle_core/metadata/user.h>

#include <doodle_dingding/client/dingding_api.h>
#include <doodle_dingding/metadata/access_token.h>
#include <doodle_dingding/metadata/attendance.h>
#include <doodle_dingding/metadata/department.h>
#include <doodle_dingding/metadata/request_base.h>

#include <boost/asio.hpp>
#include <boost/system/detail/error_code.hpp>

#include <system_error>

namespace doodle::business {

class attendance_dingding::impl {
 public:
  doodle::dingding_api_ptr client{};

  entt::handle user_handle{};
  time_point_wrap begin_time{};
  time_point_wrap end_time{};
  std::string user_id{};

  detail::attendance_interface::call_type_ptr call_fun{};
  std::queue<std::shared_ptr<std::function<void()>>> call_queue;
  std::shared_ptr<std::function<void()>> current_run{};
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

void attendance_dingding::async_run(
    const entt::handle& in_handle, const time_point_wrap& in_begin, const time_point_wrap& in_end,
    const detail::attendance_interface::call_type_ptr& in_call_type_ptr
) {
  ptr->call_queue.emplace(std::make_shared<std::function<void()>>([in_handle, in_begin, in_end, in_call_type_ptr,
                                                                   this]() {
    set_user(in_handle);
    ptr->client = g_reg()->ctx().at<dingding_api_factory>().create_api(in_handle.get<dingding::user>().company);
    set_range(in_begin, in_end);
    auto l_user   = in_handle.get<doodle::dingding::user>();
    ptr->call_fun = in_call_type_ptr;
    if (l_user.user_id.empty() && l_user.phone_number.empty()) {
      (*in_call_type_ptr)({error_enum::null_string}, {});
      ptr->current_run.reset();
      do_work();
    }

    if (l_user.user_id.empty()) {
      ptr->client->async_find_mobile_user(
          l_user.phone_number,
          [this, in_handle,
           in_call_type_ptr](const boost::system::error_code& in_code, const dingding::user_dd& in_user_dd) {
            if (in_code) {
              (*in_call_type_ptr)(in_code, {});
              ptr->current_run.reset();
              do_work();
              return;
            }
            auto& l_user = in_handle.get<doodle::dingding::user>();
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
  }));
  do_work();
}

void attendance_dingding::do_work() {
  if (!ptr->call_queue.empty() && !ptr->current_run) {
    if (ptr->call_queue.front()) {
      ptr->current_run = ptr->call_queue.front();
      (*ptr->current_run)();
      ptr->call_queue.pop();
    }
  }
}

void attendance_dingding::get_work_time() {
  ptr->client->async_get_user_updatedata_attendance_list(
      ptr->begin_time, ptr->end_time, ptr->user_id,
      [this](const boost::system::error_code& in_code, const std::vector<dingding::attendance::attendance>& in_list) {
        auto l_err_code = in_code;
        if (l_err_code) (*ptr->call_fun)(l_err_code, {});

        work_clock l_w{};
        try {
          for (const auto& item : in_list) {
            item.add_clock_data(l_w);
          }

        } catch (const doodle_error& in_err) {
          l_err_code = boost::system::error_code{error_enum::parse_string};
          DOODLE_LOG_ERROR(in_err);
        }
        (*ptr->call_fun)(l_err_code, l_w);
        ptr->current_run.reset();
        do_work();
      }
  );
}

attendance_dingding::~attendance_dingding() = default;
}  // namespace doodle::business