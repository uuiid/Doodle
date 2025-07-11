//
// Created by TD on 25-2-17.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/socket_io/core_enum.h>
namespace doodle::socket_io {
class sid_ctx;
class sid_data;

struct packet_base {
  virtual ~packet_base()           = default;
  virtual std::string dump() const = 0;
  virtual bool is_binary() const { return false; }
};

struct engine_io_packet : public packet_base {
  engine_io_packet_type type_;
  std::string message_;
  explicit engine_io_packet(engine_io_packet_type type, std::string message)
      : type_(type), message_(std::move(message)) {}
  explicit engine_io_packet(engine_io_packet_type type) : type_(type) {}
  engine_io_packet() = default;
  std::string dump() const override;
};
struct socket_io_packet : public packet_base {
  socket_io_packet_type type_;
  std::string namespace_;
  std::int64_t id_;
  std::size_t binary_count_{};
  nlohmann::json json_data_{};
  std::vector<std::string> binary_data_{};

  // 从字符串中解析
  static socket_io_packet parse(const std::string& in_str);
  /// 自动包含 engine.io 的消息头
  std::string dump() const override;
};

}  // namespace doodle::socket_io
