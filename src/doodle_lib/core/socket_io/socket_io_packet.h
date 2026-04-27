//
// Created by TD on 25-2-17.
//

#pragma once
#include <doodle_lib/core/socket_io/core_enum.h>
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::socket_io {
class sid_ctx;
class sid_data;
struct engine_io_packet;
struct socket_io_packet;
struct packet_base {
 protected:
  std::string data_;
  std::vector<std::string> binary_data_;

 public:
  explicit packet_base(const engine_io_packet& in_data);
  explicit packet_base(const socket_io_packet& in_data);
  packet_base()  = default;
  ~packet_base() = default;
  void set_data(const engine_io_packet& in_data);
  void set_data(const socket_io_packet& in_data);
  const std::string& get_dump_data() const;

  inline void set_binary_data(std::vector<std::string> in_binary_data) { binary_data_ = std::move(in_binary_data); }
  inline void add_binary_data(std::string in_binary_data) { binary_data_.push_back(std::move(in_binary_data)); }
  inline void add_binary_data(std::vector<std::string> in_binary_data) {
    for (auto& l_str : in_binary_data) binary_data_.push_back(std::move(l_str));
  }
  const std::vector<std::string>& get_binary_data() const;

  operator bool() const { return !data_.empty(); }
};

struct engine_io_packet {
  engine_io_packet_type type_;
  std::string message_;
  explicit engine_io_packet(engine_io_packet_type type, std::string message)
      : type_(type), message_(std::move(message)) {}
  explicit engine_io_packet(engine_io_packet_type type) : type_(type) {}
  engine_io_packet() = default;
  std::string dump() const;
};
struct socket_io_packet {
 public:
  socket_io_packet_type type_;
  std::string namespace_;
  std::int64_t id_;
  std::size_t binary_count_{};
  nlohmann::json json_data_{};
  std::vector<std::string> binary_data_{};
  std::string dump() const;

  // 从字符串中解析
  static socket_io_packet parse(const std::string& in_str);
  /// 自动包含 engine.io 的消息头
  const std::vector<std::string>& get_binary_data() const;
};
using packet_base_ptr = std::shared_ptr<packet_base>;
}  // namespace doodle::socket_io
