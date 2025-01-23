//
// Created by TD on 25-1-22.
//

#include "engine_io.h"

#include <magic_enum.hpp>
namespace doodle::socket_io {
void engine_io::parse(const std::string& in_data) {
  // std::int8_t l_num = in_data.front();
  // if (l_num > 0 && l_num < enum_to_num(engine_io_packet_type::noop)) {
  packet_type_ = num_to_enum<engine_io_packet_type>(in_data.front());
  if (packet_type_ == engine_io_packet_type::message) data_ = in_data.substr(1);
  // } else {
  //   packet_type_ = engine_io_packet_type::message;
  //   data_        = in_data;
  // }
}
std::string engine_io::reply(std::string&& in_data) {
  in_data.insert(in_data.begin(), char{enum_to_num(packet_type_)});
  if (packet_type_ == engine_io_packet_type::message) return in_data;
  if (packet_type_ == engine_io_packet_type::ping) return {"3probe"};
}
tl::expected<query_data, std::string> parse_query_data(const boost::urls::url& in_url) {
  query_data l_ret{};
  for (auto&& l_item : in_url.params()) {
    if (l_item.key == "sid" && l_item.has_value) l_ret.sid_ = l_item.value;
    if (l_item.key == "transport" && l_item.has_value) {
      l_ret.transport_ = magic_enum::enum_cast<transport_type>(l_item.value).value_or(transport_type::unknown);
    }
    if (l_item.key == "EIO" && l_item.has_value) l_ret.EIO_ = std::stoi(l_item.value);
  }
  if (l_ret.EIO_ != 4 || l_ret.transport_ == transport_type::unknown) return tl::make_unexpected("invalid query data");
  return {std::move(l_ret)};
}

}  // namespace doodle::socket_io