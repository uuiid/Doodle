//
// Created by TD on 2026-11-6.
//

#pragma once

#include <doodle_lib/core/face_capture/face_capture_types.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/uuid/uuid.hpp>

#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <utility>
#include <vector>

namespace doodle::face_capture {

class ws_message {
 public:
  ws_message()                                 = default;
  ws_message(const ws_message&)                = delete;
  ws_message& operator=(const ws_message&)     = delete;
  ws_message(ws_message&&) noexcept            = default;
  ws_message& operator=(ws_message&&) noexcept = default;
  virtual ~ws_message()                        = default;

  [[nodiscard]] virtual nlohmann::json to_json() const = 0;

  [[nodiscard]] virtual bool binary() const { return false; }

  [[nodiscard]] virtual bool key() const { return true; }

  [[nodiscard]] virtual std::vector<std::uint8_t> payload() const;
};

struct ws_hello : ws_message {
  ws_hello() = default;

  ws_hello(
      boost::uuids::uuid in_session_id, capture_state in_state, std::int32_t in_width, std::int32_t in_height
  )
      : session_id_(in_session_id), state_(in_state), width_(in_width), height_(in_height) {}

  boost::uuids::uuid session_id_{};
  capture_state state_{capture_state::idle};
  std::int32_t width_{0};
  std::int32_t height_{0};

  [[nodiscard]] nlohmann::json to_json() const override;
};

struct ws_avc_init : ws_message {
  explicit ws_avc_init(std::vector<std::uint8_t> in_init_segment) : init_segment_(std::move(in_init_segment)) {}

  [[nodiscard]] nlohmann::json to_json() const override;

  [[nodiscard]] bool binary() const override { return true; }

  [[nodiscard]] std::vector<std::uint8_t> payload() const override { return init_segment_; }

  [[nodiscard]] const std::vector<std::uint8_t>& init_segment() const { return init_segment_; }

 private:
  std::vector<std::uint8_t> init_segment_{};
};

struct ws_avc_fragment : ws_message {
  explicit ws_avc_fragment(fragment_t in_fragment) : fragment_(std::move(in_fragment)) {}

  [[nodiscard]] nlohmann::json to_json() const override;

  [[nodiscard]] bool binary() const override { return true; }

  [[nodiscard]] bool key() const override { return fragment_.key_; }

  [[nodiscard]] std::vector<std::uint8_t> payload() const override { return fragment_.data_; }

  [[nodiscard]] const fragment_t& fragment() const { return fragment_; }

 private:
  fragment_t fragment_{};
};

}  // namespace doodle::face_capture
