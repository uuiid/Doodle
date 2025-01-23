//
// Created by TD on 25-1-22.
//

#include "engine_io.h"
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

}  // namespace doodle::socket_io