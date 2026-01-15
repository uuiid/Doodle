//
// Created by TD on 25-7-1.
//

#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/core/bcrypt/bcrypt.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/http_method/seed_email.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

#include <cache.hpp>
#include <cache_policy.hpp>
#include <lru_cache_policy.hpp>
#include <map>
#include <string>

namespace doodle::http {

namespace {
std::string generate_reset_token() {
  static constexpr std::string_view g_random_token = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, g_random_token.size() - 1);
  std::string l_ret{};
  l_ret.resize(64);
  for (auto i = 0; i < 64; ++i) l_ret[i] = g_random_token[dis(gen)];
  return l_ret;
}
/// 生成邮件内容
std::string generate_email_content(const std::string& in_user_name, const std::string& in_token) {
  auto l_time = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()}.get_local_time();
  return fmt::format(
      R"(<p> 您好, {0}</p>
<p> 您的密码重置请求已经收到,请点击下面的链接进行密码重置:</p>
<p> <a href="{1}">{1}</a> </p>

<p> 如果您没有请求密码重置,请忽略本邮件。</p>
<p> 此链接将在2小时后过期。之后，您必须重新请求重置密码。</p>
<p> 此电子邮件于以下日期发送： {2} </p>

<p> 感谢您的使用!</p>
<p> Doodle Team</p>
)",
      in_user_name, in_token, l_time
  );
}

struct auth_reset_password_put_arg {
  std::string email;
  std::string token;
  std::string password;
  std::string password2;
  // from json
  friend void from_json(const nlohmann::json& in_json, auth_reset_password_put_arg& out_obj) {
    in_json.at("email").get_to(out_obj.email);
    in_json.at("token").get_to(out_obj.token);
    in_json.at("password").get_to(out_obj.password);
    in_json.at("password2").get_to(out_obj.password2);
  }
};
}  // namespace
class auth_reset_password::impl {
  struct cache_value {
    std::string token_;
    chrono::sys_time_pos create_time_{};
  };

 public:
  impl() = default;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_{boost::asio::make_strand(g_io_context())};

  std::map<std::string, cache_value> reset_tokens_;
  boost::asio::awaitable<std::string> get_reset_token(const std::string& in_email) {
    DOODLE_TO_EXECUTOR(strand_);
    std::string l_str{};
    if (reset_tokens_.contains(in_email)) {
      if (reset_tokens_.at(in_email).create_time_ + std::chrono::hours(2) > std::chrono::system_clock::now())
        l_str = reset_tokens_.at(in_email).token_;
      reset_tokens_.erase(in_email);
    }
    DOODLE_TO_SELF()
    co_return l_str;
  }
  boost::asio::awaitable<void> set_reset_token(const std::string& in_email, const std::string& in_token) {
    DOODLE_TO_EXECUTOR(strand_);
    reset_tokens_[in_email] = cache_value{in_token, std::chrono::system_clock::now()};
    DOODLE_TO_SELF()
    co_return;
  }
};
void auth_reset_password::init() {
  if (!pimpl_) pimpl_ = std::make_shared<impl>();
}

boost::asio::awaitable<boost::beast::http::message_generator> auth_reset_password::post(session_data_ptr in_handle) {
  auto l_email = in_handle->get_json()["email"].get<std::string>();
  default_logger_raw()->info("重置密码 {}", l_email);
  auto l_sql = g_ctx().get<sqlite_database>();
  person l_person;
  try {
    l_person = l_sql.get_person_for_email(l_email);
  } catch (const doodle_error& e) {
    throw_exception(
        http_request_error{
            boost::beast::http::status::bad_request,
            nlohmann::json{{"error", true}, {"message", "Email not listed in database."}}.dump()
        }
    );
  }
  auto l_token = generate_reset_token();
  co_await pimpl_->set_reset_token(l_email, l_token);
  auto& l_kitsu_ctx = g_ctx().get<http::kitsu_ctx_t>();
  auto l_rest_url   = fmt::format(
      "{}://{}/reset-change-password?email={}&token={}", l_kitsu_ctx.domain_protocol_, l_kitsu_ctx.domain_name_,
      l_email, l_token
  );
  if (g_ctx().contains<email::seed_email>())
    g_ctx().get<email::seed_email>()(
        "doodle 重置密码", l_person.email_, generate_email_content(l_person.get_full_name(), l_rest_url)
    );

  co_return in_handle->make_msg(nlohmann::json{{"success", "Reset token sent"}});
}
boost::asio::awaitable<boost::beast::http::message_generator> auth_reset_password::put(session_data_ptr in_handle) {
  auto l_arg   = in_handle->get_json().get<auth_reset_password_put_arg>();
  auto l_token = co_await pimpl_->get_reset_token(l_arg.email);
  if (l_token.empty()) throw_exception(http_request_error{boost::beast::http::status::bad_request, "无效的重置令牌。"});
  if (l_arg.password != l_arg.password2)
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "Passwords do not match."});
  auto l_sql          = g_ctx().get<sqlite_database>();
  auto l_person       = std::make_shared<person>(l_sql.get_person_for_email(l_arg.email));
  l_person->password_ = bcrypt::generateHash(l_arg.password);
  co_await l_sql.update(l_person);
  co_return in_handle->make_msg(nlohmann::json{{"success", "Password changed"}});
}
boost::asio::awaitable<boost::beast::http::message_generator> auth_logout::get(session_data_ptr in_handle) {
  boost::beast::http::response<boost::beast::http::string_body> l_res{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_res.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
  l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  l_res.set(boost::beast::http::field::access_control_allow_origin, "*");
  l_res.keep_alive(in_handle->keep_alive_);

  l_res.set(
      boost::beast::http::field::set_cookie, "access_token_cookie=; Max-Age=31540000; HttpOnly; Path=/; SameSite=Lax"
  );
  l_res.insert(
      boost::beast::http::field::set_cookie,
      "refresh_token_cookie=; Max-Age=31540000; HttpOnly; Path=/auth/refresh-token; SameSite=Lax"
  );
  l_res.body() = nlohmann::json{{"logout", true}}.dump();
  l_res.prepare_payload();

  co_return boost::beast::http::message_generator{std::move(l_res)};
}

}  // namespace doodle::http