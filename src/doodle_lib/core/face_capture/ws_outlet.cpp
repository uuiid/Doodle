//
// Created by TD on 2026-11-6.
//

#include "ws_outlet.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/face_capture/capture_session.h>
#include <doodle_lib/core/face_capture/ws_message.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/logger/logger.h>

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/consign.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core/buffers_prefix.hpp>
#include <boost/beast/websocket/stream_base.hpp>

#include <chrono>
#include <string>
#include <utility>

namespace doodle::face_capture {

namespace {

constexpr std::int32_t k_ws_idle_timeout_s{10};

}  // namespace

ws_outlet::ws_outlet(
    stream_t in_stream, std::int32_t in_device_index, std::shared_ptr<capture_session_holder> in_holder
)
    : stream_(std::move(in_stream)),
      session_(std::make_shared<capture_session>(capture_session::default_root())),
      holder_(std::move(in_holder)),
      device_index_(in_device_index) {}

ws_outlet::~ws_outlet() = default;

void ws_outlet::start() {
  boost::asio::co_spawn(
      stream_.get_executor(), run(),
      boost::asio::bind_cancellation_slot(
          app_base::Get().on_cancel.slot(), boost::asio::consign(boost::asio::detached, shared_from_this())
      )
  );
}

void ws_outlet::send(std::unique_ptr<ws_message> in_message) {
  if (!in_message) return;

  queue_.push(std::move(in_message));

  if (!writer_running_.exchange(true, std::memory_order_acq_rel)) {
    boost::asio::co_spawn(
        stream_.get_executor(), write_loop(),
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(), boost::asio::consign(boost::asio::detached, shared_from_this())
        )
    );
  }
}

boost::asio::awaitable<void> ws_outlet::write_loop() {
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    if (closing_) {
      queue_.clear();
      writer_running_.store(false, std::memory_order_release);
      co_return;
    }

    std::unique_ptr<ws_message> l_message{};
    if (!queue_.try_pop(l_message)) {
      writer_running_.store(false, std::memory_order_release);
      if (!queue_.empty() && !writer_running_.exchange(true, std::memory_order_acq_rel)) continue;
      co_return;
    }

    if (!l_message->key() && pending_frame()) continue;

    co_await write_message(std::move(l_message));
  }
}

bool ws_outlet::is_attached() const { return !closed_.load(std::memory_order_acquire); }

bool ws_outlet::idle() const { return !writing_.load(std::memory_order_acquire); }

bool ws_outlet::pending_frame() const { return writing_.load(std::memory_order_acquire); }

bool ws_outlet::stalled() const {
  return pending_since_ != std::chrono::steady_clock::time_point{} &&
         std::chrono::steady_clock::now() - pending_since_ >= std::chrono::milliseconds{k_stop_drain_timeout_ms};
}

void ws_outlet::mark_closed() {
  closing_ = true;
  closed_.store(true, std::memory_order_release);
  writing_.store(false, std::memory_order_release);
}

std::shared_ptr<capture_session> ws_outlet::session() const { return session_; }

boost::asio::awaitable<void> ws_outlet::run() {
  session_->attach(shared_from_this());
  if (holder_) holder_->reset(session_);

  try {
    co_await open_capture();
    co_await read_loop();
  } catch (const std::exception& in_err) {
    SPDLOG_WARN("face_capture 会话异常结束: {}", in_err.what());
  }

  co_await close_protocol();
  co_await close_impl();
}

boost::asio::awaitable<void> ws_outlet::take_over_previous() {
  if (!holder_) co_return;
  auto l_previous = holder_->session();
  if (!l_previous || l_previous == session_) co_return;
  try {
    co_await boost::asio::dispatch(
        boost::asio::bind_executor(l_previous->get_executor(), boost::asio::use_awaitable)
    );
    co_await l_previous->close();
    SPDLOG_INFO("face_capture 新连接接管摄像头, 已关闭旧会话");
  } catch (const std::exception& in_err) {
    SPDLOG_WARN("face_capture 关闭旧会话失败: {}", in_err.what());
  }
}

boost::asio::awaitable<void> ws_outlet::close_protocol() {
  if (!stream_.is_open()) co_return;
  try {
    co_await stream_.async_close(boost::beast::websocket::close_code::normal, boost::asio::use_awaitable);
  } catch (const std::exception& in_err) {
    SPDLOG_INFO("face_capture ws 关闭: {}", in_err.what());
  }
}

boost::asio::awaitable<void> ws_outlet::open_capture() {
  stream_.set_option(boost::beast::websocket::stream_base::timeout{
      .handshake_timeout = std::chrono::seconds{k_ws_idle_timeout_s},
      .idle_timeout      = std::chrono::seconds{k_ws_idle_timeout_s},
      .keep_alive_pings  = true,
  });
  try {
    co_await session_->open(capture_option{.device_index_ = device_index_});
  } catch (const std::exception& in_err) {
    SPDLOG_WARN("face_capture 打开摄像头失败: {}", in_err.what());
  }
}

boost::asio::awaitable<void> ws_outlet::close_capture() {
  try {
    co_await session_->close();
  } catch (const std::exception& in_err) {
    SPDLOG_WARN("face_capture 关闭摄像头失败: {}", in_err.what());
  }
}

boost::asio::awaitable<void> ws_outlet::read_loop() {
  while (!closed_.load(std::memory_order_acquire)) {
    read_buffer_.consume(read_buffer_.size());
    try {
      co_await stream_.async_read(read_buffer_, boost::asio::use_awaitable);
    } catch (const std::exception& in_err) {
      SPDLOG_INFO("face_capture ws 连接结束: {}", in_err.what());
      break;
    }
  }
}

boost::asio::awaitable<void> ws_outlet::close_impl() {
  mark_closed();

  co_await close_capture();

  if (holder_) holder_->reset(nullptr);
}

boost::asio::awaitable<void> ws_outlet::write_message(std::unique_ptr<ws_message> in_message) {
  if (closing_) co_return;

  writing_.store(true, std::memory_order_release);
  pending_since_ = std::chrono::steady_clock::now();

  auto l_text   = in_message->to_json().dump();
  auto l_binary = in_message->binary() ? in_message->payload() : std::vector<std::uint8_t>{};

  co_await write_raw(false, std::vector<std::uint8_t>{l_text.begin(), l_text.end()});
  if (!l_binary.empty()) co_await write_raw(true, l_binary);

  pending_since_ = {};
  writing_.store(false, std::memory_order_release);
  co_return;
}

boost::asio::awaitable<void> ws_outlet::write_raw(bool in_binary, const std::vector<std::uint8_t>& in_bytes) {
  if (closing_) co_return;

  try {
    stream_.binary(in_binary);
    co_await stream_.async_write(boost::asio::buffer(in_bytes), boost::asio::use_awaitable);
    stream_.binary(false);
  } catch (const std::exception& in_err) {
    SPDLOG_INFO("face_capture ws 写失败: {}", in_err.what());
    mark_closed();
    co_return;
  }
  co_return;
}

}  // namespace doodle::face_capture
