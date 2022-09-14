//
// Created by TD on 2022/9/9.
//
#pragma once

#include <doodle_dingding/client/client.h>
#include <nlohmann/json_fwd.hpp>
#include <doodle_core/lib_warp/entt_warp.h>

#include <doodle_dingding/metadata/access_token.h>
namespace doodle::dingding {
class access_token;
class department_query;
using dingidng_call_fun  = std::function<void(const entt::handle& in)>;
using dingidng_call_list_fun = std::function<void(const std::vector<entt::handle>& in)>;

class DOODLE_DINGDING_API dingding_api : public client {
 private:
 public:
  constexpr static const std::string_view dingding_host{"https://oapi.dingtalk.com"};

  explicit dingding_api(
      const boost::asio::any_io_executor& in_executor,
      boost::asio::ssl::context& in_ssl_context
  );
  void async_get_token(
      read_access_token_fun&& in
  );

  void async_get_departments(
      const department_query& in_query,
      const access_token& in_token,
      dingidng_call_fun&& in_fun
  );
};

}  // namespace doodle::dingding
