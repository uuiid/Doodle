#pragma once

#include "doodle_core/metadata/project.h"
#include <doodle_core/core/http_client_core.h>

#include <doodle_lib/core/http/json_body.h>

#include <boost/algorithm/string.hpp>

#include <tl/expected.hpp>

namespace doodle::metadata::kitsu {
struct task_type_t;
}

namespace doodle::kitsu {
class kitsu_client;

class kitsu_client {
  using http_client_core     = doodle::http::detail::http_client_data_base;
  using http_client_core_ptr = std::shared_ptr<http_client_core>;
  http_client_core_ptr http_client_core_ptr_{};
  std::string access_token_{};
  std::string refresh_token_{};
  std::string session_cookie_{};

  template <typename Req>
  std::decay_t<Req> header_operator_req(Req&& in_req) {
    in_req.set(boost::beast::http::field::accept, "application/json");
    in_req.set(boost::beast::http::field::content_type, "application/json");
    in_req.set(boost::beast::http::field::host, http_client_core_ptr_->server_ip_);
    in_req.set(boost::beast::http::field::authorization, "Bearer " + access_token_);
    if (!session_cookie_.empty()) {
      in_req.set(boost::beast::http::field::cookie, session_cookie_ + ";");
    }
    in_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    in_req.keep_alive(true);
    in_req.prepare_payload();
    return std::move(in_req);
  }

  template <typename Resp>
  void header_operator_resp(Resp& in_resp) {
    auto l_set_cookie = in_resp[boost::beast::http::field::set_cookie];
    if (!l_set_cookie.empty()) {
      std::vector<std::string> l_cookie;
      boost::split(l_cookie, l_set_cookie, boost::is_any_of(";"));
      for (auto& l_item : l_cookie) {
        if (l_item.starts_with("session=")) {
          session_cookie_ = l_item;
        }
      }
    }
  }

 public:
  template <typename ExecutorType>
  explicit kitsu_client(ExecutorType&& in_executor, std::string in_url)
      : http_client_core_ptr_(std::make_shared<http_client_core>(in_executor)) {
    http_client_core_ptr_->init(in_url);
  }

  ~kitsu_client() = default;

  inline void set_access_token(std::string in_token) { access_token_ = std::move(in_token); }

  struct task {
    // form json
    friend void from_json(const nlohmann::json& j, task& p) {}
  };

  boost::asio::awaitable<std::tuple<boost::system::error_code, task>> get_task(const boost::uuids::uuid& in_uuid);

  struct user_t {
    std::string phone_{};
    // form json
    friend void from_json(const nlohmann::json& j, user_t& p) { j.at("phone").get_to(p.phone_); }
  };

  boost::asio::awaitable<std::tuple<boost::system::error_code, user_t>> get_user(const boost::uuids::uuid& in_uuid);
  boost::asio::awaitable<tl::expected<std::vector<project_helper::database_t>, std::string>> get_all_project();
  boost::asio::awaitable<tl::expected<std::vector<metadata::kitsu::task_type_t>, std::string>> get_all_task_type();
};

using kitsu_client_ptr = std::shared_ptr<kitsu_client>;

}  // namespace doodle::kitsu