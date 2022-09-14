//
// Created by TD on 2022/9/9.
//

#include "dingding_api.h"
#include <doodle_dingding/configure/config.h>
#include <doodle_dingding/metadata/department.h>
#include <doodle_dingding/metadata/request_base.h>
#include <doodle_core/doodle_core.h>
#include <doodle_dingding/fmt_lib/boost_beast_fmt.h>

#include <nlohmann/json.hpp>

namespace doodle {
namespace dingding {
dingding_api::dingding_api(
    const boost::asio::any_io_executor& in_executor,
    boost::asio::ssl::context& in_ssl_context
) : client(in_executor, in_ssl_context) {
  static boost::url l_dinding{dingding_host};
  set_openssl(l_dinding.host());
}
void dingding_api::async_get_token(
    read_access_token_fun&& in
) {
  boost::url l_url{};
  boost::url l_method{"gettoken"};
  l_method.params().set("appkey", dingding_config::get().app_key);
  l_method.params().set("appsecret", dingding_config::get().app_value);

  client_ns::http_req_res<
      boost::beast::http::request<boost::beast::http::empty_body>,
      boost::beast::http::response<boost::beast::http::string_body>>
      l_http_req_res{shared_from_this()};
  l_http_req_res.req_attr.method(boost::beast::http::verb::get);

  boost::urls::resolve(
      boost::urls::url_view{dingding_host},
      l_method,
      l_http_req_res.url_attr
  );
  l_http_req_res.read_fun = [l_fun = std::move(in)](const decltype(l_http_req_res.res_attr)& in) {
    auto l_j                 = nlohmann::json::parse(in.body());
    auto l_access_token_body = access_token_body{l_j};
    if (l_access_token_body) {
      throw_exception(doodle_error{l_access_token_body});
    }
    l_fun(l_access_token_body.result_type);
  };

  run(l_http_req_res);
  //  run(l_url);
  //  return ptr->res_.body();
}
void dingding_api::async_get_departments(
    const department_query& in_query,
    const access_token& in_token,
    dingidng_call_fun&& in_fun
) {
  boost::url l_url{};
  boost::url l_method{"topapi/v2/department/listsub"};
  l_method.params().set("access_token", in_token.token);

  client_ns::http_req_res<
      boost::beast::http::request<boost::beast::http::string_body>,
      boost::beast::http::response<boost::beast::http::string_body>>
      l_http_req_res{shared_from_this()};
  l_http_req_res.req_attr.method(boost::beast::http::verb::post);
  nlohmann::json l_json          = in_query;
  l_http_req_res.req_attr.body() = l_json.dump();

  boost::urls::resolve(
      boost::urls::url_view{dingding_host},
      l_method,
      l_http_req_res.url_attr
  );
  auto l_call_fun         = std::make_shared<dingidng_call_fun>(in_fun);

  l_http_req_res.read_fun = [l_call_fun, this](const decltype(l_http_req_res.res_attr)& in) {
    if (in.body().empty())
      return;
    DOODLE_LOG_INFO(in);
    auto l_j    = nlohmann::json::parse(in.body());
    auto l_body = department_body{l_j};
    if (l_body) {
      throw_exception(doodle_error{l_body});
    }
    auto l_msg = l_body.result_type |
                 ranges::view::transform([](const department& in) -> entt::handle {
                   auto l_handle = doodle::make_handle();
                   l_handle.emplace<department>(in);
                   return l_handle;
                 }) |
                 ranges::to_vector;

    ranges::for_each(l_msg, [l_call_fun, this](const entt::handle& in) {
      boost::asio::post(
          this->get_executor(),
          [l_call_fun, in]() { (*l_call_fun)(in); }
      );
    });
  };

  run(l_http_req_res);
}
}  // namespace dingding
}  // namespace doodle
