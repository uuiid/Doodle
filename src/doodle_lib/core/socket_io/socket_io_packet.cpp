//
// Created by TD on 25-2-17.
//

#include "socket_io_packet.h"

#include <doodle_lib/core/socket_io/engine_io.h>
namespace doodle::socket_io {

const std::vector<std::string>& packet_base::get_binary_data() const {
  static std::vector<std::string> binary_data_;
  return binary_data_;
}
const std::string& packet_base::get_dump_data() const { return dump_data_; }
void packet_base::start_dump() { dump_data_ = dump(); }

socket_io_packet socket_io_packet::parse(const std::string& in_str) {
  socket_io_packet l_packet{};
  std::size_t l_pos{};
  {
    std::int32_t l_type = in_str.front() - '0';
    if (l_type < 0 || l_type > enum_to_num(socket_io_packet_type::binary_ack))
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "数据包格式错误"});
    l_packet.type_ = static_cast<socket_io_packet_type>(l_type);
  }

  if (l_packet.type_ == socket_io_packet_type::binary_ack || l_packet.type_ == socket_io_packet_type::binary_event) {
    std::size_t l_size{};
    while (in_str[++l_pos] != '-' && l_pos < in_str.size()) ++l_size;
    l_packet.binary_count_ = std::stoll(in_str.substr(1, l_size));
  }

  if (in_str[l_pos + 1] == '/') {
    auto l_begin = l_pos + 1;
    std::size_t l_size{};
    while (in_str[++l_pos] != ',' && l_pos < in_str.size()) ++l_size;
    l_packet.namespace_ = in_str.substr(l_begin, l_size);
  }
  if (l_pos == in_str.size()) return l_packet;

  if (auto l_id = in_str[l_pos + 1]; std::isdigit(l_id)) {
    auto l_begin = l_pos + 1;
    std::size_t l_size{};
    while (std::isdigit(in_str[++l_pos]) && l_pos < in_str.size()) ++l_size;
    l_packet.id_ = std::stoll(in_str.substr(l_begin, l_size));
    --l_pos;
  }
  ++l_pos;
  if (in_str.begin() + l_pos != in_str.end()) {
    if (!nlohmann::json::accept(in_str.begin() + l_pos, in_str.end()))
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "数据包格式错误"});
    l_packet.json_data_ = nlohmann::json::parse(in_str.begin() + l_pos, in_str.end());
  } else
    l_packet.json_data_ = nlohmann::json::object();
  return l_packet;
}

std::string socket_io_packet::dump() const {
  std::string l_result{std::to_string(enum_to_num(type_))};
  if (!namespace_.empty()) l_result += namespace_ + ',';

  if (type_ == socket_io_packet_type::binary_ack || type_ == socket_io_packet_type::binary_event)
    l_result += std::to_string(binary_data_.size()) + '-';
  if (type_ == socket_io_packet_type::binary_ack || type_ == socket_io_packet_type::ack)
    l_result += std::to_string(id_);

  l_result += json_data_.dump();
  return dump_message(l_result);
}
const std::vector<std::string>& socket_io_packet::get_binary_data() const { return binary_data_; }
bool socket_io_packet::is_binary() const { return is_binary(); }

std::string engine_io_packet::dump() const { return dump_message(message_, type_); }

}  // namespace doodle::socket_io