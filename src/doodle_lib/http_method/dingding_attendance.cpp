#include "dingding_attendance.h"

#include <doodle_core/metadata/attendance.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/time_tool/work_clock.h>

#include "doodle_lib/core/http/http_session_data.h"
#include "doodle_lib/core/http/http_websocket_data.h"
#include <doodle_lib/core/holidaycn_time.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/share_fun.h>

namespace doodle::http {

class dingding_attendance_impl : public std::enable_shared_from_this<dingding_attendance_impl> {
  http_session_data_ptr handle_;
  user user_;
  entt::entity user_entity_{entt::null};
  chrono::year_month_day date_;

  std::vector<attendance> attendance_list_{};

  // 钉钉客户端
  dingding::client_ptr dingding_client_;

  business::work_clock2 work_clock_;

  void find_user(const boost::uuids::uuid& in_user_id) {
    auto l_logger = handle_->logger_;
    auto l_user   = std::as_const(*g_reg()).view<const user>();
    for (auto&& [e, l_u] : l_user.each()) {
      if (l_u.id_ == in_user_id) {
        user_        = l_u;
        user_entity_ = e;
        break;
      }
    }
    if (user_entity_ == entt::null) {
      l_logger->log(log_loc(), level::err, "user {} not found", in_user_id);
      boost::system::error_code ec{boost::system::errc::bad_message, boost::system::generic_category()};
      handle_->seed_error(boost::beast::http::status::not_found, ec, "user not found");
      return;
    }

    // 寻找公司
    auto& l_d = g_ctx().get<const dingding::dingding_company>();
    if (l_d.company_info_map_.contains(user_.dingding_company_id_)) {
      dingding_client_ = l_d.company_info_map_.at(user_.dingding_company_id_).client_ptr_;
    } else {
      l_logger->log(log_loc(), level::err, "company {} not found", user_.dingding_company_id_);
      boost::system::error_code ec{boost::system::errc::bad_message, boost::system::generic_category()};
      handle_->seed_error(boost::beast::http::status::not_found, ec, "company not found");
      return;
    }

    if (user_.attendance_block_.contains(date_)) {
      auto&& l_attendance_entt = user_.attendance_block_[date_];
      auto& l_att              = std::as_const(*g_reg()).get<const attendance_block>(l_attendance_entt);
      if (chrono::system_clock::now() - l_att.update_time_.get_sys_time() < chrono::hours{1}) {
        attendance_list_ = l_att.attendance_block_;
        l_logger->log(
            log_loc(), level::info, "使用缓存数据, {} {} {}", user_.mobile_, chrono::local_days{l_att.create_date_},
            l_att.id_
        );
        send_post_result();
        return;
      }
    }

    if (user_.mobile_.empty()) {
      user_.id_           = in_user_id;
      auto l_kitsu_client = g_ctx().get<kitsu::kitsu_client_ptr>();
      // l_kitsu_client->get_user(
      //     user_.id_,
      //     boost::asio::bind_executor(
      //         g_io_context(),
      //         boost::beast::bind_front_handler(&dingding_attendance_impl::do_feach_mobile, shared_from_this())
      //     )
      // );
    } else {
      boost::asio::post(boost::asio::bind_executor(
          g_io_context(),
          boost::beast::bind_front_handler(&dingding_attendance_impl::feach_dingding, shared_from_this())
      ));
    }
  }
  void do_feach_mobile(boost::system::error_code ec, nlohmann::json l_json) {
    auto l_logger = handle_->logger_;
    if (ec) {
      l_logger->log(log_loc(), level::err, "get user failed: {}", ec.message());
      handle_->seed_error(boost::beast::http::status::internal_server_error, ec);
      return;
    }
    try {
      user_.mobile_ = l_json["phone"].get<std::string>();
    } catch (const nlohmann::json::exception& e) {
      l_logger->log(
          log_loc(), level::err, "user {} json parse error: {}", l_json["email"].get<std::string>(), e.what()
      );
      handle_->seed_error(
          boost::beast::http::status::internal_server_error, ec,
          fmt::format("{} {}", l_json["email"].get<std::string>(), e.what())
      );
      return;
    } catch (const std::exception& e) {
      l_logger->log(
          log_loc(), level::err, "user {} json parse error: {}", l_json["email"].get<std::string>(), e.what()
      );
      handle_->seed_error(
          boost::beast::http::status::internal_server_error, ec,
          fmt::format("{} {}", l_json["email"].get<std::string>(), e.what())
      );
      return;
    } catch (...) {
      l_logger->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
      ec = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
      handle_->seed_error(
          boost::beast::http::status::bad_request, ec, boost::current_exception_diagnostic_information()
      );
      return;
    }
    if (user_.mobile_.empty()) {
      l_logger->log(log_loc(), level::err, "user {} mobile is empty", l_json["email"].get<std::string>());
      handle_->seed_error(
          boost::beast::http::status::internal_server_error, ec,
          fmt::format("{} mobile is empty", l_json["email"].get<std::string>())
      );
      return;
    }
    entt::handle l_handle{};
    {
      l_handle                       = {*g_reg(), user_entity_};
      l_handle.patch<user>().mobile_ = user_.mobile_;
    }

    boost::asio::post(boost::asio::bind_executor(
        g_io_context(), boost::beast::bind_front_handler(&dingding_attendance_impl::feach_dingding, shared_from_this())
    ));
  }

  void feach_dingding() {
    if (user_.dingding_id_.empty()) {
      // dingding_client_->get_user_by_mobile(
      //     user_.mobile_,
      //     boost::asio::bind_executor(
      //         g_io_context(),
      //         boost::beast::bind_front_handler(&dingding_attendance_impl::do_feach_dingding, shared_from_this())
      //     )
      // );
    } else {
      feach_attendance();
    }
  }
  void do_feach_dingding(boost::system::error_code in_err, nlohmann::json in_json) {
    if (in_err) {
      handle_->logger_->log(log_loc(), level::err, "get user by mobile failed: {}", in_err.message());
      handle_->seed_error(
          boost::beast::http::status::internal_server_error, in_err, "无法从手机号码中获取钉钉用户信息"
      );
      return;
    }
    if (in_json.contains("result") && in_json["result"].contains("userid")) {
      user_.dingding_id_ = in_json["result"]["userid"].get<std::string>();
    } else {
      handle_->logger_->log(log_loc(), level::err, "get user by mobile failed: {}", in_json.dump());
      handle_->seed_error(boost::beast::http::status::internal_server_error, in_err, "返回用户信息错误");
      return;
    }
    feach_attendance();
  }

  void feach_attendance() {
    // dingding_client_->get_attendance_updatedata(
    //     user_.dingding_id_, chrono::local_days{date_},
    //     boost::asio::bind_executor(
    //         g_io_context(),
    //         boost::beast::bind_front_handler(&dingding_attendance_impl::do_feach_attendance, shared_from_this())
    //     )
    // );
  }

  void create_clock() {
    auto l_r = business::rules::get_default();
    for (auto&& l_work_time : l_r.work_pair_p) {
      work_clock_ += std::make_tuple(chrono::local_days{date_}, chrono::local_days{date_} + chrono::days{1});
    }

    // 排除绝对时间
    for (auto&& l_deduction : l_r.absolute_deduction[chrono::weekday{date_}.c_encoding()]) {
      work_clock_ -= std::make_tuple(
          chrono::local_days{date_} + l_deduction.first, chrono::local_days{date_} + l_deduction.second
      );
    }
  }

  void do_feach_attendance(boost::system::error_code in_err, nlohmann::json in_json) {
    if (in_err) {
      handle_->logger_->log(log_loc(), level::err, "get attendance failed: {}", in_err.message());
      handle_->seed_error(boost::beast::http::status::internal_server_error, in_err, "获取考勤信息失败");
      return;
    }

    std::vector<attendance> l_attendance_list{};
    try {
      create_clock();
      if (in_json.contains("result") && in_json["result"].contains("approve_list")) {
        for (auto&& l_obj : in_json["result"]["approve_list"]) {
          auto l_time_str = l_obj["begin_time"].get<std::string>();
          chrono::local_time_pos l_end_time{};
          chrono::local_time_pos l_begin_time{};
          {
            std::istringstream l_time_stream{l_time_str};
            l_time_stream >> chrono::parse("%F %T", l_begin_time);
            l_time_str = l_obj["end_time"].get<std::string>();
            l_time_stream.clear();
            l_time_stream.str(l_time_str);
            l_time_stream >> chrono::parse("%F %T", l_end_time);
          }
          // 重新使用开始时间和时间时间段计算时间
          chrono::hours l_duration{0};
          l_duration = chrono::floor<chrono::hours>(work_clock_(l_begin_time, l_end_time));
          l_end_time = work_clock_.next_time(
              l_begin_time, chrono::duration_cast<business::work_clock2::duration_type>(l_duration)
          );

          auto l_biz_type = l_obj["biz_type"].get<std::uint32_t>();
          auto l_type =
              (l_biz_type == 1 || l_biz_type == 2) ? attendance::att_enum::overtime : attendance::att_enum::leave;
          attendance l_attendance{
              .id_         = core_set::get_set().get_uuid(),
              .start_time_ = chrono::zoned_time<chrono::microseconds>{chrono::current_zone(), l_begin_time},
              .end_time_   = chrono::zoned_time<chrono::microseconds>{chrono::current_zone(), l_end_time},
              .remark_ =
                  fmt::format("{}-{}", l_obj["tag_name"].get<std::string>(), l_obj["sub_type"].get<std::string>()),
              .type_        = l_type,
              .dingding_id_ = l_obj["procInst_id"].get<std::string>(),
          };
          l_attendance_list.emplace_back(std::move(l_attendance));
        }
      }
    } catch (const std::exception& e) {
      handle_->logger_->log(log_loc(), level::err, "get attendance failed: {}", e.what());
      handle_->seed_error(boost::beast::http::status::internal_server_error, in_err, e.what());
      return;
    } catch (...) {
      handle_->logger_->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
      in_err = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
      handle_->seed_error(
          boost::beast::http::status::bad_request, in_err, boost::current_exception_diagnostic_information()
      );
      return;
    }

    entt::handle l_handle{};
    if (user_.attendance_block_.contains(date_) && g_reg()->valid(user_.attendance_block_[date_])) {
      l_handle = {*g_reg(), user_.attendance_block_[date_]};
    } else {
      l_handle = {*g_reg(), g_reg()->create()};
      l_handle.emplace<attendance_block>(attendance_block{
          .id_          = core_set::get_set().get_uuid(),
          .create_date_ = date_,
          .update_time_ =
              chrono::zoned_time<chrono::microseconds>{
                  chrono::current_zone(), chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now())
              },
          .user_ref_id_ = user_entity_
      });
      user_.attendance_block_[date_]                              = l_handle.entity();
      g_reg()->patch<user>(user_entity_).attendance_block_[date_] = l_handle.entity();
    }
    l_handle.patch<attendance_block>().attendance_block_ = l_attendance_list;
    attendance_list_                                     = l_attendance_list;

    send_post_result();
  }

  // 发送结果
  void send_post_result() {
    nlohmann::json l_json{};
    l_json      = attendance_list_;
    auto& l_req = handle_->request_parser_->get();
    boost::beast::http::response<boost::beast::http::string_body> l_response{
        boost::beast::http::status::ok, l_req.version()
    };
    l_response.keep_alive(l_req.keep_alive());
    l_response.set(boost::beast::http::field::content_type, "application/json");
    l_response.body() = l_json.dump();
    l_response.prepare_payload();
    handle_->seed(std::move(l_response));
  }

 public:
  explicit dingding_attendance_impl(http_session_data_ptr in_handle) : handle_(std::move(in_handle)) {}
  ~dingding_attendance_impl() = default;

  void run_post(const boost::uuids::uuid& in_user_id, const chrono::year_month_day& in_date) {
    date_ = in_date;
    find_user(in_user_id);
  }
};

boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_post(session_data_ptr in_handle) {
  auto l_logger      = in_handle->logger_;

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
    l_date_stream >> date::parse("%Y-%m-%d", l_date);
  } catch (...) {
    l_logger->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::current_exception_diagnostic_information()
    );
  }

  // 保存本次线程
  auto l_this_exe = co_await boost::asio::this_coro::executor;

  // 切换到主线程
  co_await boost::asio::post(boost::asio::bind_executor(g_thread(), boost::asio::use_awaitable));

  auto [l_user_handle, l_user] = find_user_handle(*g_reg(), l_user_id);

  if (!l_user_handle) co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "没有用户");
  auto& l_d = g_ctx().get<const dingding::dingding_company>();
  if (!l_d.company_info_map_.contains(l_user_id))
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "用户没有对应的公司");

  std::vector<attendance> l_attendance_list_{};

  if (l_user.attendance_block_.contains(l_date)) {
    auto&& l_attendance_entt = l_user.attendance_block_[l_date];
    auto& l_att              = std::as_const(*g_reg()).get<const attendance_block>(l_attendance_entt);
    if (chrono::system_clock::now() - l_att.update_time_.get_sys_time() < chrono::hours{1}) {
      l_attendance_list_ = l_att.attendance_block_;
      goto seed_success;
    }
  }

  if (l_user.mobile_.empty()) {
  }

seed_success:
  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_response.keep_alive(in_handle->keep_alive_);
  l_response.set(boost::beast::http::field::content_type, "application/json");
  nlohmann::json l_json{};
  l_json            = l_attendance_list_;
  l_response.body() = l_json.dump();
  l_response.prepare_payload();
  co_return l_response;
}

boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_get(session_data_ptr in_handle) {
  auto l_logger  = in_handle->logger_;

  auto l_date    = in_handle->capture_->get("date");
  auto l_user_id = in_handle->capture_->get("user_id");
  std::vector<chrono::year_month_day> l_date_list{};

  boost::uuids::uuid l_user_uuid{};
  try {
    chrono::year_month_day l_ymd{};
    chrono::year_month l_ym{};
    std::istringstream l_date_stream{l_date};
    l_date_stream >> date::parse("%Y-%m", l_ym);
    if (!l_date_stream.eof()) {
      l_date_stream >> date::parse("%Y-%m-%d", l_ymd);
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
  co_await boost::asio::post(boost::asio::bind_executor(g_thread(), boost::asio::use_awaitable));
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

  auto l_cs     = g_ctx().get<dingding::dingding_company>();
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
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/company",dingding_company_get
      ))

      ;
}
}  // namespace doodle::http