#include "user_http.h"

#include "doodle_core/metadata/time_point_wrap.h"
#include "doodle_core/platform/win/register_file_type.h"
#include <doodle_core/metadata/user.h>

#include "doodle_lib/core/http/http_session_data.h"
#include "doodle_lib/core/http/http_websocket_data.h"
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>
namespace doodle::http {

class user_post_impl : public std::enable_shared_from_this<user_post_impl> {
  user user_{};
  entt::entity user_entity_{entt::null};

  http_session_data_ptr handle_;

  // 钉钉客户端
  dingding::client_ptr dingding_client_;

 public:
  explicit user_post_impl(const http_session_data_ptr& in_handle) : handle_(in_handle) {}

  void run_post(boost::uuids::uuid in_user_id, boost::uuids::uuid in_company_id) {
    auto l_logger = handle_->logger_;
    if (!g_ctx().get<dingding::dingding_company>().company_info_map_.contains(in_company_id)) {
      // 未找到公司
      l_logger->log(log_loc(), level::err, "未找到公司 {}", in_company_id);
      boost::system::error_code l_ec{boost::system::errc::no_such_file_or_directory, boost::system::generic_category()};
      handle_->seed_error(boost::beast::http::status::not_found, l_ec, "未找到公司");
      return;
    }
    dingding_client_ = g_ctx().get<dingding::dingding_company>().company_info_map_.at(in_company_id).client_ptr_;

    auto l_v         = std::as_const(*g_reg()).view<const user>();
    for (auto&& [e, u] : l_v.each()) {
      if (u.id_ == in_user_id) {
        user_        = u;
        user_entity_ = e;
      }
    }

    if (user_entity_ == entt::null) {
      user_.dingding_company_id_ = in_company_id;
      user_.id_                  = in_user_id;
    }

    auto l_kitsu_client = g_ctx().get<kitsu::kitsu_client_ptr>();
    l_kitsu_client->get_user(
        user_.id_,
        boost::asio::bind_executor(
            g_io_context(), boost::beast::bind_front_handler(&user_post_impl::do_feach_mobile, shared_from_this())
        )
    );
  }

 private:
  void do_feach_mobile(boost::system::error_code ec, nlohmann::json l_json) {
    auto l_logger = handle_->logger_;
    if (ec) {
      l_logger->log(log_loc(), level::err, "get user failed: {}", ec.message());
      handle_->seed_error(boost::beast::http::status::internal_server_error, ec);
      return;
    }
    try {
      auto l_phone = l_json["phone"].get<std::string>();
      if (l_phone != user_.mobile_) {
        user_.dingding_id_.clear();
      }
      user_.mobile_ = l_phone;
    } catch (const nlohmann::json::exception& e) {
      l_logger->log(log_loc(), level::err, "user {} json parse error: 号码为空", l_json["email"].get<std::string>());
      handle_->seed_error(
          boost::beast::http::status::internal_server_error, ec,
          fmt::format("{} 号码为空 {}", l_json["email"].get<std::string>(), e.what())
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

    boost::asio::post(boost::asio::bind_executor(
        g_io_context(), boost::beast::bind_front_handler(&user_post_impl::feach_dingding, shared_from_this())
    ));
  }

  void feach_dingding() {
    if (user_.dingding_id_.empty()) {
      dingding_client_->get_user_by_mobile(
          user_.mobile_,
          boost::asio::bind_executor(
              g_io_context(), boost::beast::bind_front_handler(&user_post_impl::do_feach_dingding, shared_from_this())
          )
      );
    } else {
      send_response();
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
    send_response();
  }

  void update_user() {
    entt::handle l_h{};
    if (user_entity_ == entt::null) {
      user_entity_ = g_reg()->create();
      l_h          = entt::handle{*g_reg(), user_entity_};
      l_h.emplace<user>(user_);
    } else {
      l_h               = entt::handle{*g_reg(), user_entity_};
      l_h.patch<user>() = user_;
    }
  }

  void send_response() {
    update_user();
    nlohmann::json l_json;
    l_json["id"]          = fmt::to_string(user_.id_);
    l_json["mobile"]      = user_.mobile_;
    l_json["dingding_id"] = user_.dingding_id_;
    l_json["company"]     = fmt::to_string(user_.dingding_company_id_);
    auto l_res = boost::beast::http::response<boost::beast::http::string_body>{boost::beast::http::status::ok, 11};
    l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    l_res.set(boost::beast::http::field::content_type, "application/json");
    l_res.keep_alive(false);
    l_res.body() = l_json.dump();
    l_res.prepare_payload();
    handle_->seed(std::move(l_res));
  }
};

class user_post {
 public:
  user_post() : executor_(g_io_context().get_executor()) {}
  ~user_post()        = default;
  using executor_type = boost::asio::any_io_executor;
  boost::asio::any_io_executor executor_;
  boost::asio::any_io_executor get_executor() const { return executor_; }
  void operator()(boost::system::error_code in_error_code, const http_session_data_ptr& in_handle) const {
    auto l_logger = in_handle->logger_;
    if (in_error_code) {
      l_logger->log(log_loc(), level::err, "error: {}", in_error_code.message());
      in_handle->seed_error(boost::beast::http::status::internal_server_error, in_error_code);
      return;
    }

    auto& l_req = in_handle->get_msg_body_parser<boost::beast::http::string_body>()->request_parser_->get();
    boost::uuids::uuid l_company_id;
    boost::uuids::uuid l_user_id;
    try {
      auto l_json           = nlohmann::json::parse(l_req.body());
      auto l_company_id_str = l_json.at("company").get<std::string>();
      auto l_user_id_str    = in_handle->capture_->get("user_id");
      l_company_id          = boost::lexical_cast<boost::uuids::uuid>(l_company_id_str);
      l_user_id             = boost::lexical_cast<boost::uuids::uuid>(l_user_id_str);
    } catch (const std::exception& e) {
      l_logger->log(log_loc(), level::err, "error: {}", e.what());
      in_error_code =
          boost::system::error_code{boost::system::errc::invalid_argument, boost::system::generic_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, e.what());
      return;
    } catch (...) {
      l_logger->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
      in_handle->seed_error(
          boost::beast::http::status::bad_request, in_error_code, boost::current_exception_diagnostic_information()
      );
      return;
    }

    auto l_impl = std::make_shared<user_post_impl>(in_handle);
    l_impl->run_post(l_user_id, l_company_id);
  }
};

class user_get {
 public:
  user_get() : executor_(g_thread().get_executor()) {}
  ~user_get()         = default;
  using executor_type = boost::asio::any_io_executor;
  boost::asio::any_io_executor executor_;
  boost::asio::any_io_executor get_executor() const { return executor_; }
  void operator()(boost::system::error_code in_error_code, const http_session_data_ptr& in_handle) const {
    auto l_logger = in_handle->logger_;
    if (in_error_code) {
      l_logger->log(log_loc(), level::err, "error: {}", in_error_code.message());
      in_handle->seed_error(boost::beast::http::status::internal_server_error, in_error_code);
      return;
    }
    auto& l_req        = in_handle->request_parser_->get();

    auto l_user_id_str = in_handle->capture_->get("user_id");
    boost::uuids::uuid l_user_id;
    try {
      l_user_id = boost::lexical_cast<boost::uuids::uuid>(l_user_id_str);
    } catch (const std::exception& e) {
      l_logger->log(log_loc(), level::err, "error: {}", e.what());
      in_error_code =
          boost::system::error_code{boost::system::errc::invalid_argument, boost::system::generic_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, "错误的 uuid 格式");
      return;
    } catch (...) {
      l_logger->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
      in_handle->seed_error(
          boost::beast::http::status::bad_request, in_error_code, boost::current_exception_diagnostic_information()
      );
      return;
    }
    auto l_v = std::as_const(*g_reg()).view<const user>();

    boost::beast::http::response<boost::beast::http::string_body> l_res{
        boost::beast::http::status::ok, l_req.version()
    };
    l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    l_res.set(boost::beast::http::field::content_type, "application/json");
    l_res.keep_alive(l_req.keep_alive());

    for (auto&& [e, u] : l_v.each()) {
      if (u.id_ == l_user_id) {
        nlohmann::json l_json;
        l_json["id"]          = fmt::to_string(u.id_);
        l_json["mobile"]      = u.mobile_;
        l_json["dingding_id"] = u.dingding_id_;
        l_json["company"]     = fmt::to_string(u.dingding_company_id_);
        l_res.body()          = l_json.dump();
        l_res.prepare_payload();
        in_handle->seed(std::move(l_res));
        return;
      }
    }

    l_res.result(boost::beast::http::status::not_found);
    l_res.body() = nlohmann::json{{"code", boost::beast::http::status::not_found}, {"message", "未找到用户"}}.dump();
    l_res.prepare_payload();
    in_handle->seed(std::move(l_res));
  }
};

void reg_user_http(http_route& in_route) {
  in_route
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "api/doodle/user/{user_id}", session::make_http_reg_fun(user_get{})
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::post, "api/doodle/user/{user_id}",
          session::make_http_reg_fun<boost::beast::http::string_body>(user_post{})
      ));
}
}  // namespace doodle::http