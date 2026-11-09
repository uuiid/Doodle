//
// Created by TD on 2026-11-6.
//

#include "face_capture.h"

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/face_capture/capture_session.h>
#include <doodle_lib/core/face_capture/ws_outlet.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_session_data.h>

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/consign.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include <atomic>
#include <cstdint>
#include <memory>

namespace doodle::http::local {

class ws_stream_runner : public std::enable_shared_from_this<ws_stream_runner> {
 public:
  using stream_t = boost::beast::websocket::stream<tcp_stream_type>;

  explicit ws_stream_runner(
      stream_t in_stream, logger_ptr in_logger, std::int32_t in_device_index,
      std::shared_ptr<face_capture_session_holder> in_holder
  )
      : stream_(std::move(in_stream)),
        logger_(std::move(in_logger)),
        strand_(boost::asio::make_strand(g_io_context())),
        session_(std::make_shared<face_capture::capture_session>(face_capture::capture_session::default_root())),
        holder_(std::move(in_holder)),
        device_index_(in_device_index) {}

  void start() {
    open_.store(true);
    boost::asio::co_spawn(
        g_io_context(), run(),
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(), boost::asio::consign(boost::asio::detached, shared_from_this())
        )
    );
  }

 private:
  boost::asio::awaitable<void> run() {
    outlet_ = std::make_shared<face_capture::ws_outlet>(strand_);
    outlet_->attach(
        [self = shared_from_this()](bool in_binary, const std::uint8_t* in_data, std::size_t in_size) {
          return self->write(in_binary, in_data, in_size);
        }
    );
    session_->attach(outlet_);
    if (holder_) holder_->reset(session_);

    co_await open_capture();

    co_await read_loop();
    co_await close_impl();
  }

  boost::asio::awaitable<void> open_capture() {
    try {
      co_await session_->open(face_capture::capture_option{.device_index_ = device_index_});
    } catch (const std::exception& in_err) {
      SPDLOG_WARN("face_capture 打开摄像头失败: {}", in_err.what());
    }
  }

  boost::asio::awaitable<void> close_capture() {
    try {
      co_await session_->close();
    } catch (const std::exception& in_err) {
      SPDLOG_WARN("face_capture 关闭摄像头失败: {}", in_err.what());
    }
  }

  boost::asio::awaitable<bool> write(bool in_binary, const std::uint8_t* in_data, std::size_t in_size) {
    stream_.binary(in_binary);
    auto l_size =
        co_await stream_.async_write_some(true, boost::asio::buffer(in_data, in_size), boost::asio::use_awaitable);
    stream_.binary(false);
    co_return l_size == in_size;
  }

  boost::asio::awaitable<void> read_loop() {
    while (open_.load()) {
      read_buffer_.consume(read_buffer_.size());
      try {
        co_await stream_.async_read(read_buffer_, boost::asio::use_awaitable);
      } catch (const std::exception& in_err) {
        SPDLOG_INFO("face_capture ws 连接结束: {}", in_err.what());
        break;
      }
    }
  }

  boost::asio::awaitable<void> close_impl() {
    open_.store(false);

    co_await close_capture();

    if (holder_) holder_->reset(nullptr);

    outlet_->detach();

    if (stream_.is_open()) {
      try {
        co_await stream_.async_close(boost::beast::websocket::close_code::normal, boost::asio::use_awaitable);
      } catch (const std::exception& in_err) {
        SPDLOG_INFO("face_capture ws 关闭: {}", in_err.what());
      }
    }
  }

  stream_t stream_;
  logger_ptr logger_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  std::shared_ptr<face_capture::capture_session> session_;
  std::shared_ptr<face_capture::ws_outlet> outlet_;
  std::shared_ptr<face_capture_session_holder> holder_;
  boost::beast::flat_buffer read_buffer_;
  std::atomic_bool open_{false};
  std::int32_t device_index_{0};
};

bool face_capture_stream::has_websocket() const { return true; }

void face_capture_stream::websocket_init(session_data_ptr in_handle) {}

void face_capture_stream::websocket_callback(
    boost::beast::websocket::stream<tcp_stream_type> in_stream, session_data_ptr in_handle
) {
  runner_ = std::make_shared<ws_stream_runner>(
      std::move(in_stream), in_handle->logger_, selected_ ? selected_->index() : 0, holder_
  );
  runner_->start();
}

}  // namespace doodle::http::local
