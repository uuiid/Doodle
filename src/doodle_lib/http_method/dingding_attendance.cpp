#include "dingding_attendance.h"

#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/metadata/attendance.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/time_tool/work_clock.h>

#include "doodle_lib/core/http/http_session_data.h"
#include <doodle_lib/core/holidaycn_time.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/computing_time.h>

namespace doodle::http {
namespace {
/**
 * @brief 加班时钟, 需要排除12:00 - 13:00, 18:30 - 19:00绝对不可用时间, 其余时间均可加班
 * @param in_date 日期
 * @return 加班时钟
 */
auto create_clock_overtime(const chrono::year_month_day& in_date) {
  business::work_clock2 l_work_clock{};
  auto l_r = business::rules::get_default();
  holidaycn_time2 l_holidaycn_time{l_r.work_pair_p, g_ctx().get<kitsu_ctx_t>().front_end_root_ / "time"};
  // for (auto&& l_work_time : l_r.work_pair_p) {
  l_work_clock += std::make_tuple(chrono::local_days{in_date}, chrono::local_days{in_date} + chrono::days{1});
  // }
  // 排除绝对时间
  for (auto&& l_deduction : l_holidaycn_time.is_working_day(in_date) ? l_r.work_pair_1_ : l_r.work_pair_0_) {
    l_work_clock -= std::make_tuple(
        chrono::local_days{in_date} + l_deduction.first, chrono::local_days{in_date} + l_deduction.second
    );
  }
  return l_work_clock;
}
/**
 * @brief 休息时钟, 只需要工作日标准工作时间
 * @param in_date 日期
 * @return 调休时钟
 */
auto create_clock_leave(const chrono::year_month_day& in_date) {
  business::work_clock2 l_work_clock{};
  auto l_r = business::rules::get_default();
  // holidaycn_time2 l_holidaycn_time{l_r.work_pair_p, g_ctx().get<kitsu_ctx_t>().front_end_root_ / "time"};
  for (auto&& [ben, end] : l_r.work_pair_p) {
    l_work_clock += std::make_tuple(chrono::local_days{in_date} + ben, chrono::local_days{in_date} + end);
  }
  return l_work_clock;
}
}  // namespace

boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_create_post::post(
    session_data_ptr in_handle
) {
  auto l_logger                 = in_handle->logger_;

  auto l_json_1                 = in_handle->get_json();
  chrono::year_month_day l_date = l_json_1["work_date"].get<chrono::year_month_day>();

  auto& l_sqlite                = g_ctx().get<sqlite_database>();
  auto l_user                   = l_sqlite.get_by_uuid<person>(id_);
  auto& l_d                     = g_ctx().get<dingding::dingding_company>();
  auto l_studio                 = l_sqlite.get_by_uuid<studio>(l_user.studio_id_);

  // 查询缓存
  auto l_attends                = l_sqlite.get_attendance(l_user.uuid_id_, chrono::local_days{l_date});
  if (!l_attends.empty()) {
    auto& l_att = l_attends.front();
    if (chrono::system_clock::now() - l_att.update_time_.get_sys_time() < chrono::hours{1}) {
      nlohmann::json l_json{};
      if (l_att.type_ != attendance_helper::att_enum::max) l_json = l_attends;
      co_return in_handle->make_msg(l_json.dump());
    }
  }

  bool l_modify_user{};
  if (l_user.phone_.empty())
    co_return in_handle->logger_->error("/api/dingding/attendance {} {}", l_user.id_, "没有手机号"),
        in_handle->make_error_code_msg(boost::beast::http::status::not_found, "没有手机号");

  auto l_dingding_client = l_d.make_client(l_studio);
  if (l_user.dingding_id_.empty()) {
    l_user.dingding_id_ = co_await l_dingding_client->get_user_by_mobile(l_user.phone_);
    l_modify_user       = true;
  }

  auto l_attend =
      co_await l_dingding_client->get_attendance_updatedata(l_user.dingding_id_, chrono::local_days{l_date});

  auto l_attendance_update_list  = std::make_shared<std::vector<attendance_helper::database_t>>();
  auto l_attendance_install_list = std::make_shared<std::vector<attendance_helper::database_t>>();

  auto l_clock_overtime          = create_clock_overtime(l_date);  // 加班计算时间时钟
  auto l_clock_leave             = create_clock_leave(l_date);     // 请假计算时间时钟
  // 重新查询, 以调整协程期间可能变化的数据
  l_attends                      = l_sqlite.get_attendance(l_user.uuid_id_, chrono::local_days{l_date});
  for (auto&& l_obj : l_attend) {
    // 重新使用开始时间和时间时间段计算时间
    chrono::hours l_duration{0};
    auto l_type = (l_obj.biz_type_ == 1 || l_obj.biz_type_ == 2) ? attendance_helper::att_enum::overtime
                                                                 : attendance_helper::att_enum::leave;
    default_logger_raw()->info("考勤数据 {} {}", l_obj.begin_time_, l_obj.end_time_);
    switch (l_type) {
      case attendance_helper::att_enum::overtime:
        l_duration      = chrono::floor<chrono::hours>(l_clock_overtime(l_obj.begin_time_, l_obj.end_time_ + 1s));
        l_obj.end_time_ = l_clock_overtime.next_time(
            l_obj.begin_time_, chrono::duration_cast<business::work_clock2::duration_type>(l_duration)
        );
        break;
      case attendance_helper::att_enum::leave:
        l_duration      = chrono::floor<chrono::hours>(l_clock_leave(l_obj.begin_time_, l_obj.end_time_ + 1s));
        l_obj.end_time_ = l_clock_leave.next_time(
            l_obj.begin_time_, chrono::duration_cast<business::work_clock2::duration_type>(l_duration)
        );
        break;
      case attendance_helper::att_enum::max:
        break;
    }
    default_logger_raw()->info("修正后考勤数据 {} {}", l_obj.begin_time_, l_obj.end_time_);

    if (auto l_it = std::ranges::find_if(
            l_attends,
            [&](const attendance_helper::database_t& in_data) { return in_data.dingding_id_ == l_obj.prcoInst_id_; }
        );
        l_it != std::end(l_attends)) {
      l_it->remark_ = fmt::format("{}-{}", l_obj.tag_name_, l_obj.sub_type_), l_it->type_ = l_type;
      l_it->create_date_ = chrono::local_days{l_date};
      l_it->update_time_ = chrono::zoned_time<chrono::microseconds>{
          chrono::current_zone(), chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now())
      };
      l_attendance_update_list->emplace_back(std::move(*l_it));
      l_attends.erase(l_it);
    } else {
      attendance_helper::database_t l_attendance{
          .start_time_  = chrono::zoned_time<chrono::microseconds>{chrono::current_zone(), l_obj.begin_time_},
          .end_time_    = chrono::zoned_time<chrono::microseconds>{chrono::current_zone(), l_obj.end_time_},
          .remark_      = fmt::format("{}-{}", l_obj.tag_name_, l_obj.sub_type_),
          .type_        = l_type,
          .create_date_ = chrono::local_days{l_date},
          .update_time_ =
              chrono::zoned_time<chrono::microseconds>{
                  chrono::current_zone(), chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now())
              },
          .dingding_id_ = l_obj.prcoInst_id_,
          .person_id_   = l_user.uuid_id_
      };
      l_attendance_install_list->emplace_back(std::move(l_attendance));
    }
  }

  if (l_attendance_install_list->empty() && l_attendance_update_list->empty()) {
    attendance_helper::database_t l_attendance{
        .start_time_  = chrono::zoned_time<chrono::microseconds>{},
        .end_time_    = chrono::zoned_time<chrono::microseconds>{},
        .type_        = attendance_helper::att_enum::max,
        .create_date_ = chrono::local_days{l_date},
        .update_time_ =
            chrono::zoned_time<chrono::microseconds>{
                chrono::current_zone(), chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now())
            },
        .person_id_ = l_user.uuid_id_
    };
    l_attendance_install_list->emplace_back(std::move(l_attendance));
  }
  if (l_modify_user) co_await l_sqlite.update(std::make_shared<person>(l_user));

  if (!l_attends.empty()) {
    std::vector<std::int64_t> l_rem{};
    for (auto&& id : l_attends) {
      l_rem.emplace_back(id.id_);
    }
    co_await l_sqlite.remove<attendance_helper::database_t>(l_rem);
  }
  if (!l_attendance_update_list->empty()) {
    co_await l_sqlite.update_range<attendance_helper::database_t>(l_attendance_update_list);
  }
  if (!l_attendance_install_list->empty()) {
    co_await l_sqlite.install_range<attendance_helper::database_t>(l_attendance_install_list);
  }

  auto l_attendance_list = ranges::views::concat(*l_attendance_update_list, *l_attendance_install_list) |
                           ranges::to<std::vector<attendance_helper::database_t>>();

  co_await recomputing_time(l_user.uuid_id_, chrono::year_month{l_date.year(), l_date.month()});
  std::erase_if(l_attendance_list, [](const auto& l_attendance) {
    return l_attendance.type_ == attendance_helper::att_enum::max;
  });
  nlohmann::json l_json{};
  l_json = l_attendance_list;
  co_return in_handle->make_msg(l_json.dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_get::get(session_data_ptr in_handle) {
  if (user_id_ != person_.person_.uuid_id_) person_.check_supervisor();
  std::vector<chrono::local_days> l_date_list{};
  auto l_end = chrono::local_days{chrono::year_month_day{year_month_ / chrono::last}};
  for (auto l_day = chrono::local_days{chrono::year_month_day{year_month_ / 1}}; l_day <= l_end;
       l_day += chrono::days{1}) {
    l_date_list.emplace_back(l_day);
  }
  auto& l_sql = g_ctx().get<sqlite_database>();
  auto l_user = l_sql.get_by_uuid<person>(user_id_);
  auto l_list = l_sql.get_attendance(l_user.uuid_id_, l_date_list);
  l_list |= ranges::actions::remove_if([](const attendance_helper::database_t& in_) {
    return in_.type_ == attendance_helper::att_enum::max;
  });

  nlohmann::json l_json{};
  l_json = l_list;
  co_return in_handle->make_msg(l_json.dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_id_custom::post(
    session_data_ptr in_handle
) {
  if (id_ != person_.person_.uuid_id_) person_.check_supervisor();

  auto l_data = std::make_shared<attendance_helper::database_t>(
      std::move(in_handle->get_json().get<attendance_helper::database_t>())
  );
  if (l_data->type_ == attendance_helper::att_enum::max)
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "类型错误"});
  auto& l_sql        = g_ctx().get<sqlite_database>();
  auto l_user        = l_sql.get_by_uuid<person>(id_);
  l_data->person_id_ = l_user.uuid_id_;

  const chrono::year_month_day l_date{l_data->create_date_};
  co_await l_sql.install(l_data);
  co_await recomputing_time(l_user.uuid_id_, chrono::year_month{l_date.year(), l_date.month()});
  co_return in_handle->make_msg((nlohmann::json{} = *l_data).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_custom::put(
    session_data_ptr in_handle
) {
  if (id_ != person_.person_.uuid_id_) person_.check_supervisor();

  auto l_data = std::make_shared<attendance_helper::database_t>(
      g_ctx().get<sqlite_database>().get_by_uuid<attendance_helper::database_t>(id_)
  );
  in_handle->get_json().get_to(*l_data);

  const chrono::year_month_day l_date{l_data->create_date_};
  auto& l_sql = g_ctx().get<sqlite_database>();
  co_await l_sql.update(l_data);
  co_await recomputing_time(l_data->person_id_, chrono::year_month{l_date.year(), l_date.month()});
  co_return in_handle->make_msg((nlohmann::json{} = *l_data).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_custom::delete_(
    session_data_ptr in_handle
) {
  auto& l_sql = g_ctx().get<sqlite_database>();
  auto l_data = l_sql.get_by_uuid<attendance_helper::database_t>(id_);
  if (l_data.person_id_ != person_.person_.uuid_id_) person_.check_supervisor();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除 考勤记录 {} ", person_.person_.email_,
      person_.person_.get_full_name(), l_data.uuid_id_
  );

  co_await l_sql.remove<attendance_helper::database_t>(id_);
  co_return in_handle->make_msg((nlohmann::json{} = id_).dump());
}

}  // namespace doodle::http