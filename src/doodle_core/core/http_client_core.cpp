//
// Created by TD on 2024/3/6.
//

#include "http_client_core.h"

#include "app_base.h"
#include "exception/exception.h"

namespace doodle::http {
void http_client_ssl::set_ssl() {
  socket_->set_verify_mode(boost::asio::ssl::verify_none);
  if (!SSL_set_tlsext_host_name(socket_->native_handle(), server_ip_.c_str()))
    throw_exception(doodle_error{"SSL_set_tlsext_host_name error"});
}
}  // namespace doodle::http