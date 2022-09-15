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
  using req_type = boost::beast::http::request<boost::beast::http::empty_body>;
  using res_type = boost::beast::http::response<boost::beast::http::string_body>;

  boost::url l_url{};
  req_type l_req{};
  boost::url l_method{"gettoken"};

  l_method.params().set("appkey", dingding_config::get().app_key);
  l_method.params().set("appsecret", dingding_config::get().app_value);

  l_req.method(boost::beast::http::verb::get);
  boost::urls::resolve(
      boost::urls::url_view{dingding_host},
      l_method,
      l_url
  );
  async_write_read<res_type>(
      l_req,
      l_url,
      [l_fu = std::move(in)](
          boost::system::error_code in_code,
          const res_type& in_res_type
      ) {
        auto l_j                 = nlohmann::json::parse(in_res_type.body());
        auto l_access_token_body = access_token_body{l_j};
        if (l_access_token_body) {
          throw_exception(l_access_token_body.get_error());
        }
        l_fu(l_access_token_body.result_type());
      }
  );
}
void dingding_api::async_get_departments(
    const department_query& in_query,
    const access_token& in_token,
    dingidng_call_fun&& in_fun
) {
  boost::url l_url{};
  boost::url l_method{"topapi/v2/department/listsub"};
  l_method.params().set("access_token", in_token.token);
  using req_type = boost::beast::http::request<boost::beast::http::string_body>;
  using res_type = boost::beast::http::response<boost::beast::http::string_body>;
  req_type l_req{};

  l_req.method(boost::beast::http::verb::post);
  nlohmann::json l_json = in_query;
  l_req.body()          = l_json.dump();

  DOODLE_LOG_INFO(l_req);

  boost::urls::resolve(
      boost::urls::url_view{dingding_host},
      l_method,
      l_url
  );
  auto l_call_fun = std::make_shared<dingidng_call_fun>(in_fun);

  async_write_read<res_type>(
      l_req,
      l_url,
      [=](
          boost::system::error_code in_code,
          const res_type& in_res_type
      ) {
        DOODLE_LOG_INFO(in_res_type);
        if (in_res_type.body().empty())
          return;
        auto l_j    = nlohmann::json::parse(in_res_type.body());
        auto l_body = department_body{l_j};
        if (l_body) {
          throw_exception(l_body.get_error());
        }
        auto l_res = l_body.result_type();
        auto l_msg = l_res |
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
      }
  );
}
void dingding_api::async_get_departments_user(
    const department_query& in_query,
    const access_token& in_token,
    dingidng_call_fun&& in_fun
) {
}
}  // namespace dingding
}  // namespace doodle
