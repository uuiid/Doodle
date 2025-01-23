//
// Created by TD on 25-1-22.
//

#include "engine_io.h"
namespace doodle::socket_io {
void engine_io::parse(const std::string& in_data) {
  packet_type_ = num_to_enum<engine_io_packet_type>(in_data.front());
  if (packet_type_ == engine_io_packet_type::message) data_ = in_data.substr(1);
}

}  // namespace doodle::socket_io