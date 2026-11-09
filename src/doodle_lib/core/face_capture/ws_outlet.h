//
// Created by TD on 2026-11-6.
//

#pragma once

#include <doodle_lib/core/face_capture/face_capture_types.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/uuid/uuid.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace doodle::face_capture {

class ws_outlet : public std::enable_shared_from_this<ws_outlet> {
 public:
  using executor_type = boost::asio::strand<boost::asio::io_context::executor_type>;

  using write_fn =
      std::function<boost::asio::awaitable<bool>(bool in_binary, const std::uint8_t* in_data, std::size_t in_size)>;

  explicit ws_outlet(executor_type in_executor);

  ~ws_outlet();

  ws_outlet(const ws_outlet&)            = delete;
  ws_outlet& operator=(const ws_outlet&) = delete;

  void attach(write_fn in_write);

  void detach();

  [[nodiscard]] bool is_attached() const;

  void set_init_segment(std::vector<std::uint8_t> in_init_segment);

  void set_session_info(boost::uuids::uuid in_session_id, std::int32_t in_width, std::int32_t in_height);

  void send_hello();

  void send_event(nlohmann::json in_json);

  void send_fragment(fragment_t in_fragment);

  void notify_state(capture_state in_state);

  [[nodiscard]] bool idle() const;

  [[nodiscard]] const executor_type& get_executor() const;

 private:
  class impl;
  std::shared_ptr<impl> impl_ptr_{};
};

}  // namespace doodle::face_capture
