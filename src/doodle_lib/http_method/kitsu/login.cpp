//
// Created by TD on 25-3-6.
//

#include "doodle_core/metadata/organisation.h"
#include <doodle_core/core/bcrypt/bcrypt.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include "kitsu.h"
#include <jwt-cpp/jwt.h>
namespace doodle::http {
namespace {
struct login_data {
  std::string email_;
  std::string password_;
  bool is_trusted_;
  std::int32_t _vts_;
  // form json
  friend void from_json(const nlohmann::json& j, login_data& v) {
    j.at("email").get_to(v.email_);
    j.at("password").get_to(v.password_);
    if (j.contains("isTrusted")) j.at("isTrusted").get_to(v.is_trusted_);
    if (j.contains("_vts")) j.at("_vts").get_to(v._vts_);
  }
};
}  // namespace

boost::asio::awaitable<boost::beast::http::message_generator> authenticated_get::callback(session_data_ptr in_handle) {
  get_person(in_handle);
  nlohmann::json l_r{};
  l_r["authenticated"] = true;
  l_r["user"]          = *person_;
  auto l_org           = g_ctx().get<sqlite_database>().get_all<organisation>();
  l_r["organisation"]  = l_org.empty() ? organisation::get_default() : l_org.front();
  co_return in_handle->make_msg(l_r.dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> organisations_get::callback(session_data_ptr in_handle) {
  get_person(in_handle);
  auto l_org = g_ctx().get<sqlite_database>().get_all<organisation>();
  co_return in_handle->make_msg((nlohmann::json{l_org.empty() ? organisation::get_default() : l_org.front()}).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> auth_login_post::callback(session_data_ptr in_handle) {
  auto l_data = in_handle->get_json().get<login_data>();
  auto& l_sql = g_ctx().get<sqlite_database>();
  if (l_data.email_.empty() || l_data.password_.empty())
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "email 或 password 为空"});
  auto l_p = std::make_shared<person>(g_ctx().get<sqlite_database>().get_person_for_email(l_data.email_));
  if (!l_p->active_) throw_exception(http_request_error{boost::beast::http::status::unauthorized, "用户未激活"});

  if (l_p->login_failed_attemps_ > 5 && l_p->last_login_failed_ &&
      (l_p->last_login_failed_->get_sys_time() + 1min) > chrono::system_clock::now())
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "密码错误超过5次,请1分钟后再试"});

  if (!bcrypt::validatePassword(l_data.password_, l_p->password_)) {
    ++l_p->login_failed_attemps_;
    co_await l_sql.install(l_p);
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "密码错误"});
  }
  if (l_p->login_failed_attemps_ > 0) {
    l_p->login_failed_attemps_ = 0;
    co_await l_sql.install(l_p);
  }
  nlohmann::json l_json{};

  l_json["user"]         = *l_p;
  auto l_org             = g_ctx().get<sqlite_database>().get_all<organisation>();
  l_json["organisation"] = l_org.empty() ? organisation::get_default() : l_org.front();
  l_json["login"]        = true;
  auto l_access_token    = jwt::create()
                            .set_payload_claim("id", jwt::claim{fmt::to_string(l_p->uuid_id_)})
                            .sign(jwt::algorithm::hs256{"tset"});
  l_json["access_token"] = l_access_token;
  auto l_refresh_token   = jwt::create()
                             .set_payload_claim("id", jwt::claim{fmt::to_string(l_p->uuid_id_)})
                             .sign(jwt::algorithm::hs256{"tset"});
  l_json["refresh_token"] = l_refresh_token;
  boost::beast::http::response<boost::beast::http::string_body> l_res{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_res.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
  l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  l_res.set(boost::beast::http::field::access_control_allow_origin, "*");
  l_res.keep_alive(in_handle->keep_alive_);
  // if (req_header_[boost::beast::http::field::accept_encoding].contains("deflate")) {
  //   l_res.body() = zlib_compress(std::move(in_body));
  //   l_res.set(boost::beast::http::field::content_encoding, "deflate");
  // } else
  l_res.set(
      boost::beast::http::field::set_cookie,
      fmt::format("{}; Max-Age=31540000; HttpOnly; Path=/; SameSite=Lax", l_access_token)
  );
  l_res.set(
      boost::beast::http::field::set_cookie,
      fmt::format("{}; Max-Age=31540000; HttpOnly; Path=/auth/refresh-token; SameSite=Lax", l_refresh_token)
  );
  l_res.body() = l_json.dump();

  l_res.prepare_payload();

  co_return boost::beast::http::message_generator{std::move(l_res)};
}

void register_login(http_route& in_r) {
#ifdef DOODLE_KITSU
  in_r.reg(std::make_shared<auth_login_post>())
      .reg(std::make_shared<authenticated_get>())
      .reg(std::make_shared<organisations_get>());
#endif
}
}  // namespace doodle::http