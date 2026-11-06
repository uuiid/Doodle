//
// Created by TD on 2026-11-6.
//

#include "capture_session.h"

#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/boost_uuid_warp.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/face_capture/frame_align.h>
#include <doodle_lib/core/face_capture/media_fragment_pipeline.h>
#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/logger/logger.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/consign.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include <fmt/format.h>
#include <spdlog/fmt/ranges.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>

namespace doodle::face_capture {

namespace {

using clock_type = std::chrono::steady_clock;

std::int32_t even(std::int32_t in_value) { return std::max<std::int32_t>(in_value - in_value % 2, 2); }

std::string now_string() {
  return fmt::format(
      "{:%FT%T%Ez}", std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::system_clock::now()}
  );
}

std::int64_t elapsed_ms(clock_type::time_point in_start, clock_type::time_point in_now) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(in_now - in_start).count();
}

std::string uuid_string(boost::uuids::uuid in_id) { return fmt::format("{}", in_id); }

}  // namespace

class capture_session::impl : public std::enable_shared_from_this<capture_session::impl> {
 public:
  using stream_type = boost::beast::websocket::stream<boost::beast::tcp_stream>;

  explicit impl(FSys::path in_root)
      : root_(std::move(in_root)), strand_(boost::asio::make_strand(g_io_context())) {}

  ~impl() = default;

  boost::asio::awaitable<capture_result> start(const capture_option& in_option) {
    co_await boost::asio::this_coro::throw_if_cancelled(false);

    if (state_ == capture_state::recording || state_ == capture_state::stopping)
      co_return fail(error_code_t::already_recording);

    close_resources();

    auto l_session_id = boost::uuids::random_generator{}();
    auto l_dir        = root_ / uuid_string(l_session_id);
    if (FSys::exists(l_dir)) FSys::remove_all(l_dir);
    FSys::create_directories(l_dir);

    auto l_width  = even(in_option.width_ > 0 ? in_option.width_ : k_default_width);
    auto l_height = even(in_option.height_ > 0 ? in_option.height_ : k_default_height);

    try {
      auto l_cap = std::make_shared<cv::VideoCapture>();
      if (!l_cap->open(in_option.device_index_, cv::CAP_DSHOW)) l_cap->open(in_option.device_index_, cv::CAP_MSMF);
      if (!l_cap->isOpened()) {
        FSys::remove_all(l_dir);
        co_return fail(
            error_code_t::camera_open_failed, fmt::format("无法打开摄像头 {}", in_option.device_index_)
        );
      }

      l_cap->set(cv::CAP_PROP_FRAME_WIDTH, l_width);
      l_cap->set(cv::CAP_PROP_FRAME_HEIGHT, l_height);

      cv::Mat l_probe{};
      *l_cap >> l_probe;
      if (!l_probe.empty()) {
        l_width  = even(l_probe.cols);
        l_height = even(l_probe.rows);
      }

      width_  = l_width;
      height_ = l_height;

      fragmenter_ = h264_fragmenter::make(width_, height_);
      fragmenter_->init();
      raw_writer_ = make_raw_writer(l_dir / "raw.mkv", width_, height_);
      DOODLE_CHICK(static_cast<bool>(raw_writer_), "无法创建 raw.mkv 编码器");
      mp4_writer_ = make_mp4_writer(l_dir / "take.mp4", width_, height_, true);
      DOODLE_CHICK(static_cast<bool>(mp4_writer_), "无法创建 take.mp4 编码器");

      session_id_  = l_session_id;
      session_dir_ = l_dir;
      cap_         = std::move(l_cap);
    } catch (const std::exception& in_err) {
      close_resources();
      if (FSys::exists(l_dir)) FSys::remove_all(l_dir);
      state_ = capture_state::idle;
      co_return fail(error_code_t::camera_open_failed, in_err.what());
    }

    record_             = align_record{};
    record_.session_id_ = session_id_;
    record_.width_      = width_;
    record_.height_     = height_;
    record_.fps_        = k_fps;
    record_.started_at_ = now_string();

    seq_             = 0;
    missing_total_   = 0;
    gaps_            = {};
    last_frame_      = cv::Mat{};
    started_at_      = clock_type::now();
    next_tick_       = started_at_;
    silent_since_    = std::nullopt;
    truncated_       = false;
    silent_logged_   = false;
    state_           = capture_state::recording;

    if (outlet_) {
      outlet_->set_session_info(session_id_, width_, height_);
      outlet_->set_init_segment(fragmenter_->init_segment());
      outlet_->notify_state(to_string(capture_state::recording));
    }

    boost::asio::co_spawn(
        strand_, tick_loop(), boost::asio::consign(boost::asio::detached, this->shared_from_this())
    );

    co_return fill_result(true, capture_state::recording);
  }

  boost::asio::awaitable<capture_result> stop() {
    co_await boost::asio::this_coro::throw_if_cancelled(false);

    if (state_ != capture_state::recording) co_return fail(error_code_t::not_recording);

    state_ = capture_state::stopping;
    if (outlet_) outlet_->notify_state(to_string(capture_state::stopping));

    wait_outlet_idle();

    auto l_result       = fill_result(true, capture_state::stopped);
    l_result.truncated_ = truncated_;

    close_resources();
    state_ = capture_state::stopped;
    if (outlet_) {
      outlet_->set_init_segment({});
      outlet_->notify_state(to_string(capture_state::stopped));
    }

    co_return l_result;
  }

  void attach(std::shared_ptr<ws_outlet> in_outlet) {
    if (outlet_ && outlet_ != in_outlet) outlet_->detach();
    outlet_ = std::move(in_outlet);
    if (!outlet_) return;

    outlet_->set_session_info(session_id_, width_, height_);
    if (fragmenter_) outlet_->set_init_segment(fragmenter_->init_segment());
    outlet_->attach([self = this->shared_from_this()](const std::uint8_t* in_data, std::size_t in_size) {
      return self->write_ws(in_data, in_size);
    });
    if (state_ != capture_state::idle) outlet_->notify_state(to_string(state_));
  }

  void detach() {
    if (outlet_) outlet_->detach();
    outlet_.reset();
  }

  capture_state state() const { return state_; }

  std::uint64_t frames() const { return seq_; }

  boost::uuids::uuid session_id() const { return session_id_; }

  FSys::path session_dir() const { return session_dir_; }

  const executor_type& get_executor() const { return strand_; }

 private:
  boost::asio::awaitable<bool> write_ws(const std::uint8_t* in_data, std::size_t in_size) {
    co_await boost::asio::this_coro::throw_if_cancelled(false);
    if (!stream_) co_return false;

    co_await stream_->async_write_some(true, boost::asio::buffer(in_data, in_size), boost::asio::use_awaitable);
    co_return true;
  }

  void set_stream(std::shared_ptr<stream_type> in_stream) { stream_ = std::move(in_stream); }

  [[nodiscard]] std::shared_ptr<stream_type> stream() const { return stream_; }

  capture_result fill_result(bool in_ok, capture_state in_state) const {
    capture_result l_result{};
    l_result.ok_          = in_ok;
    l_result.state_       = in_state;
    l_result.width_       = width_;
    l_result.height_      = height_;
    l_result.fps_         = k_fps;
    l_result.frames_      = seq_ + missing_total_;
    l_result.duration_ms_ = static_cast<std::int64_t>(l_result.frames_) * k_frame_interval_ms;
    return l_result;
  }

  capture_result fail(error_code_t in_code, std::string in_message = {}) const {
    capture_result l_result{};
    l_result.ok_         = false;
    l_result.state_      = state_;
    l_result.width_      = width_;
    l_result.height_     = height_;
    l_result.fps_        = k_fps;
    l_result.frames_     = seq_ + missing_total_;
    l_result.error_code_ = in_code;
    l_result.message_    = std::move(in_message);
    return l_result;
  }

  void close_resources() {
    if (raw_writer_) {
      truncated_ = !raw_writer_->finish();
      raw_writer_.reset();
    }
    if (mp4_writer_) {
      mp4_writer_->finish();
      mp4_writer_.reset();
    }
    if (fragmenter_) {
      fragmenter_->finish();
      fragmenter_.reset();
    }
    if (cap_) {
      cap_->release();
      cap_.reset();
    }
    if (!record_.frames_.empty() && !session_dir_.empty()) {
      record_.gaps_        = gaps_;
      record_.duration_ms_ = static_cast<std::int64_t>(seq_ + missing_total_) * k_frame_interval_ms;
      record_.truncated_   = truncated_;
      try {
        write_align(record_, session_dir_);
      } catch (const std::exception& in_err) {
        SPDLOG_WARN("写 align.json 失败: {}", in_err.what());
      }
    }
    last_frame_ = cv::Mat{};
  }

  void wait_outlet_idle() {
    auto l_deadline = clock_type::now() + std::chrono::milliseconds{k_stop_drain_timeout_ms};
    while (outlet_ && !outlet_->idle() && clock_type::now() < l_deadline) {
      boost::asio::steady_timer{strand_, std::chrono::milliseconds{2}}.wait();
    }
  }

  boost::asio::awaitable<void> tick_loop() {
    boost::asio::steady_timer l_timer{strand_};
    while (state_ == capture_state::recording) {
      auto l_now = clock_type::now();
      if (l_now > next_tick_ + std::chrono::milliseconds{k_frame_interval_ms * 10}) next_tick_ = l_now;

      l_timer.expires_at(next_tick_);
      co_await l_timer.async_wait(boost::asio::use_awaitable);
      if (state_ != capture_state::recording) break;

      tick_once();
      next_tick_ += std::chrono::milliseconds{k_frame_interval_ms};
    }
  }

  void tick_once() {
    auto l_now = clock_type::now();

    cv::Mat l_grab{};
    bool l_has_new_frame{false};
    if (cap_) {
      try {
        *cap_ >> l_grab;
        l_has_new_frame = !l_grab.empty() && l_grab.type() == CV_8UC3;
      } catch (const std::exception&) {
        l_has_new_frame = false;
      }
    }

    if (l_has_new_frame && l_grab.cols == width_ && l_grab.rows == height_) {
      l_grab.copyTo(last_frame_);
    } else if (l_has_new_frame && last_frame_.empty()) {
      width_  = even(l_grab.cols);
      height_ = even(l_grab.rows);
      l_grab(cv::Rect{0, 0, width_, height_}).copyTo(last_frame_);
    }

    if (last_frame_.empty()) {
      if (!silent_logged_ && l_now - started_at_ > std::chrono::milliseconds{k_silent_timeout_ms}) {
        silent_logged_ = true;
        SPDLOG_WARN("采集会话 {} 摄像头无可用帧", uuid_string(session_id_));
      }
      return;
    }

    if (silent_since_) {
      auto l_silent_ms = elapsed_ms(*silent_since_, l_now);
      if (l_silent_ms >= k_silent_timeout_ms) {
        auto l_missing = static_cast<std::uint64_t>(l_silent_ms / k_frame_interval_ms);
        gaps_.push_back(
            gap_range{.from_seq_ = seq_, .to_seq_ = seq_ + l_missing - 1, .missing_ = l_missing}
        );
        missing_total_ += l_missing;
      }
      silent_since_.reset();
    }

    if (!l_has_new_frame && !silent_since_) silent_since_ = l_now;

    silent_logged_ = false;
    emit_frame(l_now);
  }

  void emit_frame(clock_type::time_point in_now) {
    record_.frames_.push_back(align_frame{.seq_ = seq_, .host_pts_us_ = elapsed_ms(started_at_, in_now) * 1000});

    try {
      raw_writer_->push(last_frame_);
    } catch (const std::exception& in_err) {
      SPDLOG_WARN("raw.mkv 写入失败: {}", in_err.what());
      truncated_ = true;
    }
    try {
      mp4_writer_->push(last_frame_);
    } catch (const std::exception& in_err) {
      SPDLOG_WARN("take.mp4 写入失败: {}", in_err.what());
    }

    if (outlet_ && outlet_->is_attached()) {
      try {
        fragmenter_->push(last_frame_);
        for (auto& l_fragment : fragmenter_->take_fragments()) outlet_->send_fragment(std::move(l_fragment));
      } catch (const std::exception& in_err) {
        SPDLOG_WARN("推流编码失败: {}", in_err.what());
      }
    }

    ++seq_;
  }

  FSys::path root_{};
  FSys::path session_dir_{};
  const executor_type strand_;

  capture_state state_{capture_state::idle};
  std::shared_ptr<cv::VideoCapture> cap_{};
  std::shared_ptr<h264_fragmenter> fragmenter_{};
  std::shared_ptr<raw_writer> raw_writer_{};
  std::shared_ptr<video_writer> mp4_writer_{};
  std::shared_ptr<ws_outlet> outlet_{};

  boost::uuids::uuid session_id_{};
  std::int32_t width_{0};
  std::int32_t height_{0};
  std::uint64_t seq_{0};

  cv::Mat last_frame_{};
  clock_type::time_point started_at_{};
  std::optional<clock_type::time_point> silent_since_{};
  clock_type::time_point next_tick_{};
  bool silent_logged_{false};
  bool truncated_{false};

  align_record record_{};
  std::vector<gap_range> gaps_{};
  std::uint64_t missing_total_{0};
  std::shared_ptr<stream_type> stream_{};
};

capture_session::capture_session(FSys::path in_root)
    : impl_ptr_(std::make_shared<impl>(std::move(in_root))) {}

capture_session::~capture_session() = default;

std::shared_ptr<capture_session> capture_session::make(const FSys::path& in_root) {
  return std::make_shared<capture_session>(in_root);
}

std::shared_ptr<capture_session> capture_session::instance() {
  static std::mutex l_mutex{};
  static std::shared_ptr<capture_session> l_instance{};

  std::unique_lock l_lock{l_mutex};
  if (!l_instance) l_instance = make(default_root());
  return l_instance;
}

std::vector<camera_info> capture_session::list_devices() {
  std::vector<camera_info> l_result{};
  for (std::int32_t l_index = 0; l_index < 8; ++l_index) {
    cv::VideoCapture l_cap{};
    if (!l_cap.open(l_index, cv::CAP_DSHOW)) l_cap.open(l_index, cv::CAP_MSMF);
    if (!l_cap.isOpened()) break;

    camera_info l_info{};
    l_info.index_   = l_index;
    l_info.name_    = fmt::format("camera_{}", l_index);
    l_info.backend_ = l_cap.getBackendName();
    l_info.width_   = static_cast<std::int32_t>(l_cap.get(cv::CAP_PROP_FRAME_WIDTH));
    l_info.height_  = static_cast<std::int32_t>(l_cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    l_cap.release();
    l_result.emplace_back(std::move(l_info));
  }
  return l_result;
}

FSys::path capture_session::default_root() {
  return core_set::get_set().get_cache_root(FSys::path{"face_capture"} / "sessions");
}

boost::asio::awaitable<capture_result> capture_session::start(const capture_option& in_option) {
  return impl_ptr_->start(in_option);
}

boost::asio::awaitable<capture_result> capture_session::stop() { return impl_ptr_->stop(); }

void capture_session::attach(std::shared_ptr<ws_outlet> in_outlet) { impl_ptr_->attach(std::move(in_outlet)); }

void capture_session::detach() { impl_ptr_->detach(); }

capture_state capture_session::state() const { return impl_ptr_->state(); }

std::uint64_t capture_session::frames() const { return impl_ptr_->frames(); }

boost::uuids::uuid capture_session::session_id() const { return impl_ptr_->session_id(); }

FSys::path capture_session::session_dir() const { return impl_ptr_->session_dir(); }

const capture_session::executor_type& capture_session::get_executor() const { return impl_ptr_->get_executor(); }

}  // namespace doodle::face_capture
