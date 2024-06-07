#include "dingding_attendance.h"

#include <doodle_core/metadata/attendance.h>
#include <doodle_core/metadata/user.h>

#include "doodle_lib/core/http/http_session_data.h"
#include "doodle_lib/core/http/http_websocket_data.h"
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>

namespace doodle::http {

class dingding_attendance_impl : public std::enable_shared_from_this<dingding_attendance_impl> {
  http_session_data_ptr handle_;
  user user_;
  entt::entity user_entity_{entt::null};
  chrono::year_month_day date_;

  // 钉钉客户端
  dingding::client_ptr dingding_client_;

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
    if (user_.id_ == boost::uuids::nil_uuid() || user_.mobile_.empty()) {
      user_.id_           = in_user_id;
      auto l_kitsu_client = g_ctx().get<kitsu::kitsu_client_ptr>();
      l_kitsu_client->get_user(
          user_.id_,
          boost::asio::bind_executor(
              g_io_context(),
              boost::beast::bind_front_handler(&dingding_attendance_impl::do_feach_mobile, shared_from_this())
          )
      );
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
    // 创建用户
    if (user_entity_ == entt::null) {
      l_handle = {*g_reg(), g_reg()->create()};
      l_handle.emplace<user>(user_);
    } else  // 存在用户则修改
    {
      l_handle                       = {*g_reg(), user_entity_};
      l_handle.patch<user>().mobile_ = user_.mobile_;
    }
    user_entity_ = l_handle.entity();

    if (user_.attendance_block_.contains(date_)) {
      for (auto&& l_attendance : user_.attendance_block_[date_]) {
        g_reg()->destroy(l_attendance);
      }
    }

    boost::asio::post(boost::asio::bind_executor(
        g_io_context(), boost::beast::bind_front_handler(&dingding_attendance_impl::feach_dingding, shared_from_this())
    ));
  }

  void feach_dingding() {
    if (user_.dingding_id_.empty()) {
      dingding_client_->get_user_by_mobile(
          user_.mobile_,
          boost::asio::bind_executor(
              g_io_context(),
              boost::beast::bind_front_handler(&dingding_attendance_impl::do_feach_dingding, shared_from_this())
          )
      );
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
  }

  void feach_attendance() {
    dingding_client_->get_attendance_updatedata(
        user_.dingding_id_, chrono::local_days{date_},
        boost::asio::bind_executor(
            g_io_context(),
            boost::beast::bind_front_handler(&dingding_attendance_impl::do_feach_attendance, shared_from_this())
        )
    );
  }
  void do_feach_attendance(boost::system::error_code in_err, nlohmann::json in_json) {
    if (in_err) {
      handle_->logger_->log(log_loc(), level::err, "get attendance failed: {}", in_err.message());
      handle_->seed_error(boost::beast::http::status::internal_server_error, in_err, "获取考勤信息失败");
      return;
    }
    std::vector<attendance> l_attendance_list{};
    try {
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
              .user_ref_id_ = user_entity_,
              .update_time_ =
                  chrono::zoned_time<chrono::microseconds>{
                      chrono::current_zone(), chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now())
                  },
              .dingding_id_ = l_obj["procInst_id"].get<std::string>(),
          };
          l_attendance_list.emplace_back(std::move(l_attendance));
        }
      }
    } catch (const std::exception& e) {
      handle_->logger_->log(log_loc(), level::err, "get attendance failed: {}", e.what());
      handle_->seed_error(boost::beast::http::status::internal_server_error, in_err, e.what());
      return;
    }

    std::vector<entt::entity> l_attendance_entity_list{l_attendance_list.size()};
    g_reg()->create(l_attendance_entity_list.begin(), l_attendance_entity_list.end());
    g_reg()->insert<attendance>(
        l_attendance_entity_list.begin(), l_attendance_entity_list.end(), l_attendance_list.begin()
    );
    user_.attendance_block_[date_]                              = l_attendance_entity_list;
    g_reg()->patch<user>(user_entity_).attendance_block_[date_] = l_attendance_entity_list;

    nlohmann::json l_json{};
    l_json      = l_attendance_list;
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

  void run_post(
      const boost::uuids::uuid& in_user_id, const chrono::year_month_day& in_date, const dingding::client_ptr& in_client
  ) {
    dingding_client_ = in_client;
    find_user(in_user_id);
    date_ = in_date;
  }
};

class dingding_attendance_get {
 public:
  dingding_attendance_get() : executor_(g_thread().get_executor()) {}
  ~dingding_attendance_get() = default;
  using executor_type        = boost::asio::any_io_executor;
  boost::asio::any_io_executor executor_;
  boost::asio::any_io_executor get_executor() const { return executor_; }
  void operator()(boost::system::error_code in_error_code, const http_session_data_ptr& in_handle) const {
    auto l_logger = in_handle->logger_;
    if (in_error_code) {
      l_logger->log(log_loc(), level::err, "error: {}", in_error_code.message());
      in_handle->seed_error(boost::beast::http::status::internal_server_error, in_error_code);
      return;
    }
    auto l_view    = std::as_const(*g_reg()).view<const user>();
    auto l_date    = in_handle->capture_->get("date");
    auto l_user_id = in_handle->capture_->get("user_id");
    chrono::year_month_day l_ymd{};
    boost::uuids::uuid l_user_uuid{};
    try {
      std::istringstream l_date_stream{l_date};
      l_date_stream >> date::parse("%Y-%m-%d", l_ymd);
      l_user_uuid = boost::lexical_cast<boost::uuids::uuid>(l_user_id);
    } catch (const std::exception& e) {
      l_logger->log(log_loc(), level::err, "url parse error: {}", e.what());
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, e.what());
      return;
    }
    // res
    auto& l_req = in_handle->request_parser_->get();

    boost::beast::http::response<boost::beast::http::string_body> l_response{
        boost::beast::http::status::ok, in_handle->request_parser_->get().version()
    };
    l_response.keep_alive(l_req.keep_alive());
    l_response.set(boost::beast::http::field::content_type, "application/json");

    // find user
    bool is_find = false;
    for (auto&& [e, l_u] : l_view.each()) {
      if (l_u.id_ == l_user_uuid) {
        auto l_user = l_u;
        is_find     = true;
        if (l_user.attendance_block_.contains(l_ymd)) {
          nlohmann::json l_json{};
          l_json            = l_user.attendance_block_[l_ymd];
          l_response.body() = l_json.dump();
        } else {
          l_response.body() = "[]";
        }
        break;
      }
    }

    if (!is_find) {
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
      in_handle->seed_error(boost::beast::http::status::not_found, in_error_code, "not found user or date");
      return;
    }
    l_response.prepare_payload();
    in_handle->seed(std::move(l_response));
  }
};

class dingding_company_get {
 public:
  dingding_company_get() : executor_(g_thread().get_executor()) {}
  ~dingding_company_get() = default;
  using executor_type     = boost::asio::any_io_executor;
  boost::asio::any_io_executor executor_;
  boost::asio::any_io_executor get_executor() const { return executor_; }
  void operator()(boost::system::error_code in_error_code, const http_session_data_ptr& in_handle) const {
    auto l_logger = in_handle->logger_;
    if (in_error_code) {
      l_logger->log(log_loc(), level::err, "error: {}", in_error_code.message());
      in_handle->seed_error(boost::beast::http::status::internal_server_error, in_error_code);
      return;
    }
    auto l_cs = g_ctx().get<dingding::dingding_company>();
    nlohmann::json l_json{};
    for (auto&& [l_key, l_value] : l_cs.company_info_map_) {
      l_json.emplace_back(l_value);
    }
    auto& l_req = in_handle->request_parser_->get();
    boost::beast::http::response<boost::beast::http::string_body> l_response{
        boost::beast::http::status::ok, l_req.version()
    };
    l_response.keep_alive(l_req.keep_alive());
    l_response.set(boost::beast::http::field::content_type, "application/json");
    l_response.body() = l_json.dump();
    l_response.prepare_payload();
    in_handle->seed(std::move(l_response));
  }
};

class dingding_attendance_post {
 public:
  dingding_attendance_post() : executor_(g_thread().get_executor()) {}
  ~dingding_attendance_post() = default;
  using executor_type         = boost::asio::any_io_executor;
  boost::asio::any_io_executor executor_;
  boost::asio::any_io_executor get_executor() const { return executor_; }
  void operator()(boost::system::error_code in_error_code, const http_session_data_ptr& in_handle) const {
    auto l_logger = in_handle->logger_;
    if (in_error_code) {
      l_logger->log(log_loc(), level::err, "error: {}", in_error_code.message());
      in_handle->seed_error(boost::beast::http::status::internal_server_error, in_error_code);
      return;
    }
    auto l_req                   = in_handle->get_msg_body_parser<boost::beast::http::string_body>();

    auto l_computing_time_id_str = in_handle->capture_->get("user_id");

    auto l_body                  = l_req->request_parser_->get().body();
    if (!nlohmann::json::accept(l_body)) {
      l_logger->log(log_loc(), level::err, "url parse error: {}", l_body);
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, l_body);
      return;
    }

    boost::uuids::uuid l_computing_time_id{}, l_company{};
    chrono::year_month_day l_date{};
    dingding::client_ptr l_dingding_client{};

    try {
      auto l_json         = nlohmann::json::parse(l_body);
      auto l_date_str     = l_json["work_date"].get<std::string>();
      l_company           = boost::lexical_cast<boost::uuids::uuid>(l_json["company"].get<std::string>());
      l_computing_time_id = boost::lexical_cast<boost::uuids::uuid>(l_computing_time_id_str);
      std::istringstream l_date_stream{l_date_str};
      l_date_stream >> date::parse("%Y-%m-%d", l_date);

    } catch (const std::exception& e) {
      l_logger->log(log_loc(), level::err, "url parse error: {}", e.what());
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, e.what());
      return;
    }
    auto& l_cs = g_ctx().get<dingding::dingding_company>();
    if (l_cs.company_info_map_.contains(boost::lexical_cast<boost::uuids::uuid>(l_company))) {
      l_dingding_client = l_cs.company_info_map_[boost::lexical_cast<boost::uuids::uuid>(l_company)].client_ptr_;
    } else {
      l_logger->log(log_loc(), level::err, "company not found: {}", l_company);
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, "company not found");
      return;
    }

    auto l_impl = std::make_shared<dingding_attendance_impl>(in_handle);
    l_impl->run_post(l_computing_time_id, l_date, l_dingding_client);
  }
};

void reg_dingding_attendance(http_route& in_route) {
  in_route
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::post, "api/doodle/attendance/{user_id}",
          session::make_http_reg_fun<boost::beast::http::string_body>(dingding_attendance_post{})
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "api/doodle/attendance/{user_id}/{date}",
          session::make_http_reg_fun<boost::beast::http::string_body>(dingding_attendance_get{})
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "api/doodle/company", session::make_http_reg_fun(dingding_company_get{})
      ))

      ;
}
}  // namespace doodle::http