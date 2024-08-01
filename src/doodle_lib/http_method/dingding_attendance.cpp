#include "dingding_attendance.h"

#include <doodle_core/metadata/attendance.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/time_tool/work_clock.h>

#include "doodle_lib/core/http/http_session_data.h"
#include <doodle_lib/core/holidaycn_time.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/share_fun.h>

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
} // namespace

boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_post(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

  auto l_user_id_str = in_handle->capture_->get("user_id");

  if (in_handle->content_type_ != detail::content_type::application_json) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "content type is not json");
  }

  boost::uuids::uuid l_user_id{};
  chrono::year_month_day l_date{};

  try {
    auto l_json     = std::get<nlohmann::json>(in_handle->body_);
    auto l_date_str = l_json["work_date"].get<std::string>();
    l_user_id       = boost::lexical_cast<boost::uuids::uuid>(l_user_id_str);
    std::istringstream l_date_stream{l_date_str};
    l_date_stream >> chrono::parse("%Y-%m-%d", l_date);
  } catch (...) {
    l_logger->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
    co_return in_handle->make_error_code_msg(
      boost::beast::http::status::bad_request, boost::current_exception_diagnostic_information()
    );
  }

  // 保存本次线程
  auto l_this_exe = co_await boost::asio::this_coro::executor;

  // 切换到主线程
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));

  auto [l_user_handle, l_user] = find_user_handle(*g_reg(), l_user_id);

  if (!l_user_handle) co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "没有用户");
  auto& l_d = g_ctx().get<const dingding::dingding_company>();
  if (!l_d.company_info_map_.contains(l_user.dingding_company_id_))
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "用户没有对应的公司");

  std::vector<attendance> l_attendance_list{};

  if (l_user.attendance_block_.contains(l_date)) {
    auto&& l_attendance_entt = l_user.attendance_block_[l_date];
    auto& l_att              = std::as_const(*g_reg()).get<const attendance_block>(l_attendance_entt);
    if (chrono::system_clock::now() - l_att.update_time_.get_sys_time() < chrono::hours{1}) {
      co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));
      boost::beast::http::response<boost::beast::http::string_body> l_response{
          boost::beast::http::status::ok, in_handle->version_
      };
      l_response.keep_alive(in_handle->keep_alive_);
      l_response.set(boost::beast::http::field::content_type, "application/json");
      nlohmann::json l_json{};
      l_json            = l_att.attendance_block_;
      l_response.body() = l_json.dump();
      l_response.prepare_payload();
      co_return l_response;
    }
  }

  // 切换回来
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));
  bool l_modify_user{};
  if (l_user.mobile_.empty()) {
    auto l_kitsu_client   = g_ctx().get<kitsu::kitsu_client_ptr>();
    auto [l_e2, l_mobile] = co_await l_kitsu_client->get_user(l_user_id);
    if (l_e2) co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, l_e2.message());
    l_user.mobile_ = l_mobile.phone_;
    l_modify_user  = true;
  }

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

  try {
    auto l_clock = create_clock(l_date);
    for (auto&& l_obj : l_attend) {
      // 重新使用开始时间和时间时间段计算时间
      chrono::hours l_duration{0};
      l_duration      = chrono::floor<chrono::hours>(l_clock(l_obj.begin_time_, l_obj.end_time_));
      l_obj.end_time_ =
          l_clock.next_time(l_obj.begin_time_, chrono::duration_cast<business::work_clock2::duration_type>(l_duration));

      auto l_type =
          (l_obj.biz_type_ == 1 || l_obj.biz_type_ == 2) ? attendance::att_enum::overtime : attendance::att_enum::leave;
      attendance l_attendance{
          .id_ = core_set::get_set().get_uuid(),
          .start_time_ = chrono::zoned_time<chrono::microseconds>{chrono::current_zone(), l_obj.begin_time_},
          .end_time_ = chrono::zoned_time<chrono::microseconds>{chrono::current_zone(), l_obj.end_time_},
          .remark_ = fmt::format("{}-{}", l_obj.tag_name_, l_obj.sub_type_),
          .type_ = l_type,
          .dingding_id_ = l_obj.prcoInst_id_
      };
      l_attendance_list.emplace_back(std::move(l_attendance));
    }
  } catch (const std::exception& e) {
    in_handle->logger_->log(log_loc(), level::err, "get attendance failed: {}", e.what());
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, e.what());
  } catch (...) {
    in_handle->logger_->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
    auto in_err = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, in_err.message());
  }

  // 切换到主线程
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));

  if (l_modify_user) {
    l_user_handle.patch<user>().mobile_      = l_user.mobile_;
    l_user_handle.patch<user>().dingding_id_ = l_user.dingding_id_;
  }

  entt::handle l_handle{};
  if (l_user.attendance_block_.contains(l_date) && g_reg()->valid(l_user.attendance_block_[l_date])) {
    l_handle = {*g_reg(), l_user.attendance_block_[l_date]};
  } else {
    l_handle = {*g_reg(), g_reg()->create()};
    l_handle.emplace<attendance_block>(attendance_block{
        .id_ = core_set::get_set().get_uuid(),
        .create_date_ = l_date,
        .update_time_ =
        chrono::zoned_time<chrono::microseconds>{
            chrono::current_zone(), chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now())
        },
        .user_ref_id_ = l_user_handle
    });
    l_user.attendance_block_[l_date]                      = l_handle.entity();
    l_user_handle.patch<user>().attendance_block_[l_date] = l_handle.entity();
  }
  l_handle.patch<attendance_block>().attendance_block_ = l_attendance_list;

  // 切换回来
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));

  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_response.keep_alive(in_handle->keep_alive_);
  l_response.set(boost::beast::http::field::content_type, "application/json");
  nlohmann::json l_json{};
  l_json            = l_attendance_list;
  l_response.body() = l_json.dump();
  l_response.prepare_payload();
  co_return l_response;
}

boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_get(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

  auto l_date    = in_handle->capture_->get("date");
  auto l_user_id = in_handle->capture_->get("user_id");
  std::vector<chrono::year_month_day> l_date_list{};

  boost::uuids::uuid l_user_uuid{};
  try {
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
    l_user_uuid = boost::lexical_cast<boost::uuids::uuid>(l_user_id);
  } catch (...) {
    l_logger->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "参数错误");
  }
  auto l_this_exe = co_await boost::asio::this_coro::executor;

  // 转到主线程
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
  auto l_view = std::as_const(*g_reg()).view<const user>();
  std::vector<attendance> l_r{};
  bool is_find = false;
  for (auto&& [e, l_u] : l_view.each()) {
    if (l_u.id_ == l_user_uuid) {
      auto l_user = l_u;
      is_find     = true;
      for (auto&& l_ymd : l_date_list) {
        if (l_user.attendance_block_.contains(l_ymd)) {
          for (auto&& i :
               std::as_const(*g_reg()).get<const attendance_block>(l_user.attendance_block_[l_ymd]).attendance_block_)
            // l_json.emplace_back(i);
            l_r.emplace_back(i);
        }
      }
      break;
    }
  }
  // 转入 this_exe
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));

  if (!is_find) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "用户不存在");
  }

  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, in_handle->version_
  };
  nlohmann::json l_json{};
  l_json            = l_r;
  l_response.body() = l_json.dump();
  l_response.keep_alive(in_handle->keep_alive_);
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.prepare_payload();
  co_return std::move(l_response);
}

boost::asio::awaitable<boost::beast::http::message_generator> dingding_company_get(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

  auto l_cs = g_ctx().get<dingding::dingding_company>();
  nlohmann::json l_json{};
  for (auto&& [l_key, l_value] : l_cs.company_info_map_) {
    l_json.emplace_back(l_value);
  }
  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_response.keep_alive(in_handle->keep_alive_);
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = l_json.dump();
  l_response.prepare_payload();
  co_return std::move(l_response);
}

void reg_dingding_attendance(http_route& in_route) {
  in_route
      .reg(std::make_shared<http_function>(
        boost::beast::http::verb::post, "api/doodle/attendance/{user_id}", dingding_attendance_post
      ))
      .reg(std::make_shared<http_function>(
        boost::beast::http::verb::get, "api/doodle/attendance/{user_id}/{date}", dingding_attendance_get
      ))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/company", dingding_company_get
      ));
}
} // namespace doodle::http