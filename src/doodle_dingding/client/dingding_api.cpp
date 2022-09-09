//
// Created by TD on 2022/9/9.
//

#include "dingding_api.h"

namespace doodle {
namespace dingding {
dingding_api::dingding_api(
    const boost::asio::any_io_executor& in_executor,
    boost::asio::ssl::context& in_ssl_context
) : client(in_executor, in_ssl_context) {
  static boost::url l_dinding{dingding_host};
  set_openssl(l_dinding.host());
}
std::string dingding_api::gettoken() {
  //  boost::url l_url{};
  //  boost::url l_method{"gettoken"};
  //  l_method.params().set("appkey", dingding_config::get().app_key);
  //  l_method.params().set("appsecret", dingding_config::get().app_value);
  //
  //  boost::urls::resolve(boost::urls::url_view{dingding_host}, l_method, l_url);
  //
  //  //  DOODLE_LOG_INFO(l_url.string());
  //  //  DOODLE_LOG_INFO(ptr->req_);
  //  //  DOODLE_LOG_INFO(l_url.remove_origin().string());
  //
  //  run(l_url);
  //  return ptr->res_.body();
  return {};
}
}  // namespace dingding
}  // namespace doodle
