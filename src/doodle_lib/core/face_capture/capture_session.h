//
// Created by TD on 2026-11-6.
//

#pragma once

#include <doodle_lib/core/face_capture/face_capture_types.h>
#include <doodle_lib/core/face_capture/media_fragment_pipeline.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio/awaitable.hpp>
#include <boost/uuid/uuid.hpp>

#include <memory>
#include <vector>

namespace doodle::face_capture {

class capture_session;
class ws_outlet;

class capture_session_holder {
 public:
  capture_session_holder();

  [[nodiscard]] std::shared_ptr<capture_session> session() const;

  void reset(std::shared_ptr<capture_session> in_session);

 private:
  struct impl;
  std::shared_ptr<impl> impl_ptr_{};
};

class capture_session : public std::enable_shared_from_this<capture_session> {
 public:
  using executor_type = boost::asio::strand<boost::asio::io_context::executor_type>;

  explicit capture_session(FSys::path in_root);

  ~capture_session();

  capture_session(const capture_session&)            = delete;
  capture_session& operator=(const capture_session&) = delete;

  static std::vector<camera_info> list_devices();

  static FSys::path default_root();

  boost::asio::awaitable<capture_status> open(const capture_option& in_option);

  boost::asio::awaitable<capture_status> start_record();

  boost::asio::awaitable<capture_status> stop();

  boost::asio::awaitable<capture_status> close();

  void attach(std::shared_ptr<ws_outlet> in_outlet);

  void detach();

  [[nodiscard]] std::shared_ptr<ws_outlet> outlet() const;

  capture_state state() const;

  std::uint64_t frames() const;

  boost::uuids::uuid session_id() const;

  FSys::path session_dir() const;

  const executor_type& get_executor() const;

 private:
  class impl;
  std::shared_ptr<impl> impl_ptr_{};
};

}  // namespace doodle::face_capture
