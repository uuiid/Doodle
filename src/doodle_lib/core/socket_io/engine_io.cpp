//
// Created by TD on 25-2-17.
//

#include "engine_io.h"

#include <magic_enum/magic_enum_all.hpp>



namespace doodle::socket_io {

query_data parse_query_data(const boost::urls::url& in_url) {
  query_data l_ret{.transport_ = transport_type::unknown};
  for (auto&& l_item : in_url.params()) {
    if (l_item.key == "sid" && l_item.has_value) l_ret.sid_ = from_uuid_str(l_item.value);
    if (l_item.key == "transport" && l_item.has_value) {
      l_ret.transport_ = magic_enum::enum_cast<transport_type>(l_item.value).value_or(transport_type::unknown);
    }
    if (l_item.key == "EIO" && l_item.has_value) {
      if (!std::isdigit(l_item.value[0]))
        throw_exception(http_request_error{boost::beast::http::status::bad_request, "无效的请求"});
      l_ret.EIO_ = std::stoi(l_item.value);
    }
  }
  if (l_ret.EIO_ != 4 || l_ret.transport_ == transport_type::unknown)
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "无效的请求"});
  return l_ret;
}




}  // namespace doodle::socket_io