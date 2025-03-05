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
#include <doodle_lib/http_method/computing_time.h>

namespace doodle::http {
namespace {
auto create_clock(const chrono::year_month_day& in_date) {
  business::work_clock2 l_work_clock{};
  auto l_r = business::rules::get_default();
  for (auto&& l_work_time : l_r.work_pair_p) {
    l_work_clock += std::make_tuple(chrono::local_days{in_date}, chrono::local_days{in_date} + chrono::days{1});
  }

  // 排除绝对时间
  for (auto&& l_deduction : l_r.absolute_deduction[chrono::weekday{in_date}.c_encoding()]) {
    l_work_clock -= std::make_tuple(
        chrono::local_days{in_date} + l_deduction.first, chrono::local_days{in_date} + l_deduction.second
    );
  }
  return l_work_clock;
}
}  // namespace

boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_post(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

  if (in_handle->content_type_ != detail::content_type::application_json) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "content type is not json");
  }

  auto l_json_1                = std::get<nlohmann::json>(in_handle->body_);
  boost::uuids::uuid l_user_id = boost::lexical_cast<uuid>(in_handle->capture_->get("user_id"));
  chrono::year_month_day l_date =
      parse_time<chrono::year_month_day>(l_json_1["work_date"].get<std::string>(), "%Y-%m-%d");

  auto& l_sqlite = g_ctx().get<sqlite_database>();
  auto l_user    = g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_user_id);
  auto& l_d      = g_ctx().get<const dingding::dingding_company>();
  if (l_user.dingding_company_id_.is_nil() && !l_d.company_info_map_.contains(l_user.dingding_company_id_))
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "用户没有对应的公司");

  // 查询缓存
  auto l_attends = l_sqlite.get_attendance(l_user.id_, chrono::local_days{l_date});
  if (!l_attends.empty()) {
    auto& l_att = l_attends.front();
    if (chrono::system_clock::now() - l_att.update_time_.get_sys_time() < chrono::hours{1}) {
      nlohmann::json l_json{};
      if (l_att.type_ != attendance_helper::att_enum::max) l_json = l_attends;
      co_return in_handle->make_msg(l_json.dump());
    }
  }

  bool l_modify_user{};
  if (l_user.mobile_.empty())
    co_return in_handle->logger_->error("/api/dingding/attendance {} {}", l_user.id_, "没有手机号"),
        in_handle->make_error_code_msg(boost::beast::http::status::not_found, "没有手机号");

  auto l_dingding_client =
      g_ctx().get<const dingding::dingding_company>().company_info_map_.at(l_user.dingding_company_id_).client_ptr;
  if (l_user.dingding_id_.empty()) {
    auto [l_e3, l_dingding_id] = co_await l_dingding_client->get_user_by_mobile(l_user.mobile_);
    if (l_e3) co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, l_e3.message());
    l_user.dingding_id_ = l_dingding_id;
    l_modify_user       = true;
  }

  auto [l_e4, l_attend] =
      co_await l_dingding_client->get_attendance_updatedata(l_user.dingding_id_, chrono::local_days{l_date});

  if (l_e4) co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, l_e4.message());
  auto l_attendance_list = std::make_shared<std::vector<attendance_helper::database_t>>();

  auto l_clock           = create_clock(l_date);
  for (auto&& l_obj : l_attend) {
    // 重新使用开始时间和时间时间段计算时间
    chrono::hours l_duration{0};
    l_duration = chrono::floor<chrono::hours>(l_clock(l_obj.begin_time_, l_obj.end_time_));
    l_obj.end_time_ =
        l_clock.next_time(l_obj.begin_time_, chrono::duration_cast<business::work_clock2::duration_type>(l_duration));

    auto l_type = (l_obj.biz_type_ == 1 || l_obj.biz_type_ == 2) ? attendance_helper::att_enum::overtime
                                                                 : attendance_helper::att_enum::leave;

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
      l_attendance_list->emplace_back(std::move(*l_it));
      l_attends.erase(l_it);
    } else {
      attendance_helper::database_t l_attendance{
          .uuid_id_     = core_set::get_set().get_uuid(),
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
          .user_ref     = l_user.id_
      };
      l_attendance_list->emplace_back(std::move(l_attendance));
    }
  }

  if (l_attendance_list->empty()) {
    attendance_helper::database_t l_attendance{
        .uuid_id_     = core_set::get_set().get_uuid(),
        .start_time_  = chrono::zoned_time<chrono::microseconds>{},
        .end_time_    = chrono::zoned_time<chrono::microseconds>{},
        .type_        = attendance_helper::att_enum::max,
        .create_date_ = chrono::local_days{l_date},
        .update_time_ =
            chrono::zoned_time<chrono::microseconds>{
                chrono::current_zone(), chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now())
            },
        .user_ref = l_user.id_
    };
    l_attendance_list->emplace_back(std::move(l_attendance));
  }
  if (l_modify_user) co_await l_sqlite.install(std::make_shared<user_helper::database_t>(l_user));

  if (!l_attends.empty()) {
    auto l_rem = std::make_shared<std::vector<std::int64_t>>();
    for (auto&& id : l_attends) {
      l_rem->emplace_back(id.id_);
    }
    co_await l_sqlite.remove<attendance_helper::database_t>(l_rem);
  }
  if (!l_attendance_list->empty()) {
    co_await l_sqlite.install_range<attendance_helper::database_t>(l_attendance_list);
  }

  co_await recomputing_time(
      std::make_shared<user_helper::database_t>(l_user), chrono::year_month{l_date.year(), l_date.month()}
  );
  std::erase_if(*l_attendance_list, [](const auto& l_attendance) {
    return l_attendance.type_ == attendance_helper::att_enum::max;
  });
  nlohmann::json l_json{};
  l_json = *l_attendance_list;
  co_return in_handle->make_msg(l_json.dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_get(session_data_ptr in_handle) {
  std::vector<chrono::local_days> l_date_list{};
  boost::uuids::uuid l_user_uuid = from_uuid_str(in_handle->capture_->get("user_id"));
  {
    auto l_date = in_handle->capture_->get("date");
    chrono::year_month_day l_ymd{};
    chrono::year_month l_ym{};
    std::istringstream l_date_stream{l_date};
    l_date_stream >> chrono::parse("%Y-%m", l_ym);
    if (!l_date_stream.eof()) {
      l_date_stream >> chrono::parse("%Y-%m-%d", l_ymd);
      l_date_list.emplace_back(l_ymd);
    } else {
      auto l_end = chrono::local_days{chrono::year_month_day{l_ym / chrono::last}};
      for (auto l_day = chrono::local_days{chrono::year_month_day{l_ym / 1}}; l_day <= l_end;
           l_day += chrono::days{1}) {
        l_date_list.emplace_back(l_day);
      }
    }
  }

  auto& l_sql = g_ctx().get<sqlite_database>();
  auto l_user = l_sql.get_by_uuid<user_helper::database_t>(l_user_uuid);
  auto l_list = l_sql.get_attendance(l_user.id_, l_date_list);
  l_list |= ranges::actions::remove_if([](const attendance_helper::database_t& in_) {
    return in_.type_ == attendance_helper::att_enum::max;
  });

  nlohmann::json l_json{};
  l_json = l_list;
  co_return in_handle->make_msg(l_json.dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> dingding_company_get(session_data_ptr in_handle) {
  auto l_cs = g_ctx().get<dingding::dingding_company>();
  nlohmann::json l_json{};
  for (auto&& [l_key, l_value] : l_cs.company_info_map_) {
    l_json.emplace_back(l_value);
  }
  co_return in_handle->make_msg(l_json.dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_custom_add(
    session_data_ptr in_handle
) {
  if (in_handle->content_type_ != http::detail::content_type::application_json)
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "请求格式不正确"});

  auto l_user_id = from_uuid_str(in_handle->capture_->get("user_id"));
  auto l_data    = std::make_shared<attendance_helper::database_t>(
      std::move(std::get<nlohmann::json>(in_handle->body_).get<attendance_helper::database_t>())
  );
  if (l_data->type_ == attendance_helper::att_enum::max)
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "类型错误"});
  auto& l_sql      = g_ctx().get<sqlite_database>();
  auto l_user      = std::make_shared<user_helper::database_t>(l_sql.get_by_uuid<user_helper::database_t>(l_user_id));
  l_data->uuid_id_ = l_user->uuid_id_;
  l_data->user_ref = l_user->id_;
  l_data->update_time_ = chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now());
  const chrono::year_month_day l_date{l_data->create_date_};
  co_await l_sql.install(l_data);
  co_await recomputing_time(l_user, chrono::year_month{l_date.year(), l_date.month()});
  co_return in_handle->make_msg((nlohmann::json{} = *l_data).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_custom_modify(
    session_data_ptr in_handle
) {
  if (in_handle->content_type_ != http::detail::content_type::application_json)
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "请求格式不正确"});

  auto l_id   = from_uuid_str(in_handle->capture_->get("id"));
  auto l_data = std::make_shared<attendance_helper::database_t>(
      g_ctx().get<sqlite_database>().get_by_uuid<attendance_helper::database_t>(l_id)
  );
  std::get<nlohmann::json>(in_handle->body_).get_to(*l_data);
  l_data->update_time_ = chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now());
  const chrono::year_month_day l_date{l_data->create_date_};
  auto& l_sql = g_ctx().get<sqlite_database>();
  co_await l_sql.install(l_data);
  auto l_user = std::make_shared<user_helper::database_t>(
      l_sql.get_by_uuid<user_helper::database_t>(l_sql.id_to_uuid<user_helper::database_t>(l_data->user_ref))
  );
  co_await recomputing_time(l_user, chrono::year_month{l_date.year(), l_date.month()});
  co_return in_handle->make_msg((nlohmann::json{} = *l_data).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_custom_delete(
    session_data_ptr in_handle
) {
  auto l_id   = from_uuid_str(in_handle->capture_->get("id"));
  auto& l_sql = g_ctx().get<sqlite_database>();
  co_await l_sql.remove<attendance_helper::database_t>(std::make_shared<uuid>(l_id));
  co_return in_handle->make_msg((nlohmann::json{} = l_id).dump());
}
void reg_dingding_attendance(http_route& in_route) {
  in_route
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post, "api/doodle/attendance/{user_id}", dingding_attendance_post
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post, "api/doodle/attendance/{user_id}/custom", dingding_attendance_custom_add
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::put, "api/doodle/attendance/custom/{id}", dingding_attendance_custom_modify
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::delete_, "api/doodle/attendance/custom/{id}", dingding_attendance_custom_delete
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::get, "api/doodle/attendance/{user_id}/{date}", dingding_attendance_get
          )
      );
}
}  // namespace doodle::http