//
// Created by TD on 2026-11-6.
//

#include "capture_session.h"

#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/boost_uuid_warp.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/face_capture/frame_align.h>
#include <doodle_lib/core/face_capture/frame_filter.h>
#include <doodle_lib/core/face_capture/media_fragment_pipeline.h>
#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/logger/logger.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/consign.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/post.hpp>
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
#include <condition_variable>
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
  explicit impl(FSys::path in_root)
      : root_(std::move(in_root)), strand_(boost::asio::make_strand(g_io_context())) {}

  ~impl() = default;

  boost::asio::awaitable<capture_status> open(const capture_option& in_option) {
    co_await boost::asio::this_coro::throw_if_cancelled(false);

    DOODLE_CHICK_HTTP(
        record_state_ != capture_state::recording && record_state_ != capture_state::stopping, conflict,
        "已经在录制中"
    );

    if (stream_state_ == capture_state::previewing) co_return make_status(capture_state::previewing);

    close_stream();

    auto l_width  = even(in_option.width_ > 0 ? in_option.width_ : k_default_width);
    auto l_height = even(in_option.height_ > 0 ? in_option.height_ : k_default_height);

    auto l_cap = std::make_shared<cv::VideoCapture>();
    if (!l_cap->open(in_option.device_index_, cv::CAP_DSHOW)) l_cap->open(in_option.device_index_, cv::CAP_MSMF);
    DOODLE_CHICK_HTTP(l_cap->isOpened(), unprocessable_entity, "无法打开摄像头 {}", in_option.device_index_);

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
    cap_    = std::move(l_cap);

    reset_stream_encoders();

    reset_counters(clock_type::now());
    stream_state_ = capture_state::previewing;
    record_state_ = capture_state::idle;

    if (outlet_) {
      outlet_->set_session_info(session_id_, width_, height_);
      outlet_->send_hello();
    }

    auto l_self  = this->shared_from_this();
    read_thread_ = std::thread{[l_self] { l_self->read_loop(); }};

    co_return make_status(capture_state::previewing);
  }

  boost::asio::awaitable<capture_status> start_record() {
    co_await boost::asio::this_coro::throw_if_cancelled(false);

    DOODLE_CHICK_HTTP(stream_state_ == capture_state::previewing, conflict, "摄像头未打开");
    DOODLE_CHICK_HTTP(
        record_state_ != capture_state::recording && record_state_ != capture_state::stopping, conflict,
        "已经在录制中"
    );

    close_record();

    auto l_session_id = boost::uuids::random_generator{}();
    auto l_dir        = root_ / uuid_string(l_session_id);
    if (FSys::exists(l_dir)) FSys::remove_all(l_dir);
    FSys::create_directories(l_dir);

    auto l_mp4 = std::make_shared<mp4_writer>(l_dir / "take.mp4", width_, height_, true);
    DOODLE_CHICK_HTTP(l_mp4->init(), unprocessable_entity, "无法创建 take.mp4 编码器: {}", l_dir.string());
    mp4_writer_ = std::move(l_mp4);

    session_id_  = l_session_id;
    session_dir_ = l_dir;

    record_             = align_record{};
    record_.session_id_ = session_id_;
    record_.width_      = width_;
    record_.height_     = height_;
    record_.fps_        = k_fps;
    record_.started_at_ = now_string();

    reset_recording_counters(clock_type::now());
    record_state_ = capture_state::recording;

    if (outlet_) {
      outlet_->set_session_info(session_id_, width_, height_);
      outlet_->set_init_segment(fragmenter_->init_segment());
    }

    co_return make_status(capture_state::recording);
  }

  boost::asio::awaitable<void> finish_record() {
    drain_filter();
    co_await wait_outlet_idle();
    close_record();
  }

  boost::asio::awaitable<capture_status> stop() {
    co_await boost::asio::this_coro::throw_if_cancelled(false);

    DOODLE_CHICK_HTTP(record_state_ == capture_state::recording, conflict, "当前没有在录制");

    record_state_ = capture_state::stopping;

    co_await finish_record();

    auto l_result = make_status(capture_state::previewing);
    l_result.truncated_ = truncated_;

    record_state_ = capture_state::stopped;

    co_return l_result;
  }

  boost::asio::awaitable<capture_status> close() {
    co_await boost::asio::this_coro::throw_if_cancelled(false);

    auto l_result = make_status(capture_state::idle);
    if (record_state_ == capture_state::recording) {
      record_state_ = capture_state::stopping;
      co_await finish_record();
      l_result      = make_status(capture_state::previewing);
      record_state_ = capture_state::stopped;
    }

    close_stream();

    stream_state_ = capture_state::idle;
    if (outlet_) outlet_->set_init_segment({});

    co_return l_result;
  }

  void attach(std::shared_ptr<ws_outlet> in_outlet) {
    if (outlet_ && outlet_ != in_outlet) outlet_->detach();
    outlet_ = std::move(in_outlet);
    if (!outlet_) return;

    outlet_->set_session_info(session_id_, width_, height_);
    if (fragmenter_) outlet_->set_init_segment(fragmenter_->init_segment());
  }

  void detach() {
    if (outlet_) outlet_->detach();
    outlet_.reset();
  }

  capture_state state() const { return display_state(); }

  std::uint64_t frames() const { return seq_; }

  boost::uuids::uuid session_id() const { return session_id_; }

  FSys::path session_dir() const { return session_dir_; }

  const executor_type& get_executor() const { return strand_; }

  [[nodiscard]] std::shared_ptr<ws_outlet> outlet() const { return outlet_; }

 private:
  capture_state display_state() const {
    if (record_state_ != capture_state::idle) return record_state_;
    return stream_state_;
  }

  capture_status make_status(capture_state in_state) const {
    capture_status l_status{};
    l_status.state_           = in_state;
    l_status.width_           = width_;
    l_status.height_          = height_;
    l_status.fps_             = k_fps;
    l_status.frames_          = seq_;
    l_status.raw_frames_      = orig_seq_;
    l_status.inserted_frames_ = 0;
    l_status.source_fps_      = source_fps();
    l_status.duration_ms_     = static_cast<std::int64_t>(seq_) * k_frame_interval_ms;
    l_status.truncated_       = truncated_;
    return l_status;
  }

  double source_fps() const {
    if (source_frames_ < 2) return 0.0;
    if (source_last_ms_ <= 0) return 0.0;
    return static_cast<double>(source_frames_ - 1) * 1000.0 / static_cast<double>(source_last_ms_);
  }

  void reset_stream_encoders() {
    fragmenter_ = std::make_shared<h264_fragmenter>(width_, height_);
    fragmenter_->init();

    filter_ = std::make_shared<frame_filter>(
        frame_filter_option{.width_ = width_, .height_ = height_, .graph_ = fmt::format("fps={}", k_fps)}
    );
  }

  void reset_counters(clock_type::time_point in_now) {
    seq_             = 0;
    orig_seq_        = 0;
    preview_seq_     = 0;
    source_frames_   = 0;
    source_last_ms_  = 0;
    gaps_            = {};
    started_at_      = in_now;
    last_frame_at_   = std::nullopt;
    silent_logged_   = false;
    truncated_       = false;
    read_failures_   = 0;
    read_stop_       = false;
    read_finished_   = false;
  }

  void reset_recording_counters(clock_type::time_point in_now) {
    seq_            = 0;
    orig_seq_       = 0;
    preview_seq_    = 0;
    source_frames_  = 0;
    source_last_ms_ = 0;
    gaps_           = {};
    started_at_     = in_now;
    last_frame_at_  = std::nullopt;
    silent_logged_  = false;
    truncated_      = false;
  }

  void close_record() {
    if (mp4_writer_) {
      mp4_writer_->finish();
      mp4_writer_.reset();
    }
    if (!record_.frames_.empty() && !session_dir_.empty()) {
      record_.gaps_            = gaps_;
      record_.raw_frames_      = orig_seq_;
      record_.inserted_frames_ = 0;
      record_.source_fps_      = source_fps();
      record_.duration_ms_     = static_cast<std::int64_t>(seq_) * k_frame_interval_ms;
      record_.truncated_       = truncated_;
      try {
        write_align(record_, session_dir_);
      } catch (const std::exception& in_err) { SPDLOG_WARN("写 align.json 失败: {}", in_err.what()); }
    }
  }

  void close_stream() {
    if (read_thread_.joinable()) {
      release_camera();
      std::unique_lock l_lock{read_mutex_};
      read_cv_.wait_for(
          l_lock, std::chrono::milliseconds{k_stop_drain_timeout_ms}, [this] { return read_finished_; }
      );
      l_lock.unlock();
      if (!read_finished_) {
        SPDLOG_WARN("摄像头读线程未退出, 放弃等待, 线程随进程结束");
        truncated_ = true;
        read_thread_.detach();
      } else {
        read_thread_.join();
      }
    }
    close_record();
    if (fragmenter_) {
      fragmenter_->finish();
      fragmenter_.reset();
    }
    filter_.reset();
    if (cap_) {
      cap_->release();
      cap_.reset();
    }
  }

  boost::asio::awaitable<void> wait_outlet_idle() {
    auto l_deadline = clock_type::now() + std::chrono::milliseconds{k_stop_drain_timeout_ms};
    boost::asio::steady_timer l_timer{strand_};
    while (outlet_ && !outlet_->idle() && clock_type::now() < l_deadline) {
      l_timer.expires_after(std::chrono::milliseconds{2});
      co_await l_timer.async_wait(boost::asio::use_awaitable);
    }
  }

  void read_loop() {
    while (true) {
      {
        std::unique_lock l_lock{read_mutex_};
        if (read_stop_) break;
      }

      auto l_cap = cap_;
      if (!l_cap) break;

      cv::Mat l_grab{};
      try {
        *l_cap >> l_grab;
      } catch (const std::exception& in_err) {
        SPDLOG_WARN("读取摄像头帧失败: {}", in_err.what());
        break;
      }

      if (l_grab.empty() || l_grab.type() != CV_8UC3) {
        std::unique_lock l_lock{read_mutex_};
        if (read_stop_) break;
        ++read_failures_;
        if (read_failures_ % k_fps == 0) SPDLOG_WARN("摄像头连续返回空帧 {} 次", read_failures_);
        continue;
      }
      read_failures_ = 0;

      boost::asio::post(
          strand_,
          [self = this->shared_from_this(), l_mat = std::move(l_grab), l_now = clock_type::now()]() mutable {
            self->on_capture(std::move(l_mat), l_now);
          }
      );
    }

    std::unique_lock l_lock{read_mutex_};
    read_finished_ = true;
    read_cv_.notify_all();
  }

  void request_read_stop() {
    {
      std::unique_lock l_lock{read_mutex_};
      read_stop_ = true;
    }
    read_cv_.notify_all();
  }

  void release_camera() {
    request_read_stop();
    if (cap_) cap_->release();
  }

  boost::asio::awaitable<void> wait_read_thread() {
    if (!read_thread_.joinable()) co_return;
    auto l_deadline = clock_type::now() + std::chrono::milliseconds{k_stop_drain_timeout_ms};
    while (clock_type::now() < l_deadline) {
      {
        std::unique_lock l_lock{read_mutex_};
        if (read_finished_) break;
      }
      co_await boost::asio::steady_timer{strand_, std::chrono::milliseconds{2}}.async_wait(
          boost::asio::use_awaitable
      );
    }

    std::unique_lock l_lock{read_mutex_};
    if (!read_finished_) {
      SPDLOG_WARN("摄像头读线程未在 {} ms 内退出, 放弃等待", k_stop_drain_timeout_ms);
      truncated_ = true;
      co_return;
    }
    l_lock.unlock();
  }

  void drain_filter() {
    if (!filter_) return;
    filter_->flush();
    drain_filter_frames();
  }

  void drain_filter_frames() {
    for (auto& l_frame : filter_->take_frames()) {
      const cv::Mat& l_mat  = l_frame.mat_;
      const bool l_recording = record_state_ == capture_state::recording || record_state_ == capture_state::stopping;

      if (l_recording && mp4_writer_) {
        try {
          mp4_writer_->push(l_mat, seq_);
        } catch (const std::exception& in_err) {
          SPDLOG_WARN("take.mp4 写入失败: {}", in_err.what());
          truncated_ = true;
        }
        ++seq_;
      }

      auto l_seq = l_recording ? (seq_ > 0 ? seq_ - 1 : 0) : preview_seq_++;

      if (outlet_ && outlet_->is_attached() && fragmenter_) {
        try {
          fragmenter_->push(l_mat, l_seq);
          for (auto& l_fragment : fragmenter_->take_fragments()) outlet_->send_fragment(std::move(l_fragment));
        } catch (const std::exception& in_err) {
          SPDLOG_WARN("推流编码失败: {}", in_err.what());
        }
      }
    }
  }

  void on_capture(cv::Mat in_mat, clock_type::time_point in_now) {
    if (stream_state_ != capture_state::previewing) return;
    DOODLE_CHICK(
        in_mat.cols >= width_ && in_mat.rows >= height_, "摄像头帧尺寸 {}x{} 小于采集尺寸 {}x{}", in_mat.cols,
        in_mat.rows, width_, height_
    );
    if (in_mat.cols != width_ || in_mat.rows != height_) in_mat = in_mat(cv::Rect{0, 0, width_, height_});

    const auto l_wall_ms = elapsed_ms(started_at_, in_now);
    on_silence(l_wall_ms);
    ++source_frames_;
    source_last_ms_ = l_wall_ms;

    if (record_state_ == capture_state::recording) {
      record_.frames_.push_back(align_frame{.seq_ = seq_, .orig_seq_ = orig_seq_, .wall_ms_ = l_wall_ms});
      ++seq_;
      ++orig_seq_;
    }

    filter_->push(in_mat, l_wall_ms * 1000);
    drain_filter_frames();
  }

  void on_silence(std::int64_t in_wall_ms) {
    if (last_frame_at_) {
      const auto l_gap_ms = in_wall_ms - *last_frame_at_;
      if (l_gap_ms >= k_silent_timeout_ms) {
        gaps_.push_back(
            gap_range{
                .wall_ms_begin_ = *last_frame_at_, .wall_ms_end_ = in_wall_ms, .duration_ms_ = l_gap_ms
            }
        );
        SPDLOG_WARN("摄像头静默 {} ms", l_gap_ms);
      }
    }
    last_frame_at_ = in_wall_ms;
  }

  FSys::path root_{};
  FSys::path session_dir_{};
  const executor_type strand_;

  capture_state stream_state_{capture_state::idle};
  capture_state record_state_{capture_state::idle};
  std::shared_ptr<cv::VideoCapture> cap_{};
  std::shared_ptr<h264_fragmenter> fragmenter_{};
  std::shared_ptr<video_writer> mp4_writer_{};
  std::shared_ptr<ws_outlet> outlet_{};
  std::shared_ptr<frame_filter> filter_{};

  boost::uuids::uuid session_id_{};
  std::int32_t width_{0};
  std::int32_t height_{0};
  std::uint64_t seq_{0};
  std::uint64_t orig_seq_{0};
  std::uint64_t preview_seq_{0};
  std::uint64_t source_frames_{0};
  std::int64_t source_last_ms_{0};

  clock_type::time_point started_at_{};
  std::optional<std::int64_t> last_frame_at_{};
  bool silent_logged_{false};
  bool truncated_{false};

  std::thread read_thread_{};
  std::mutex read_mutex_{};
  std::condition_variable read_cv_{};
  bool read_stop_{false};
  bool read_finished_{false};
  std::uint64_t read_failures_{0};

  align_record record_{};
  std::vector<gap_range> gaps_{};
};

capture_session::capture_session(FSys::path in_root)
    : impl_ptr_(std::make_shared<impl>(std::move(in_root))) {}

capture_session::~capture_session() = default;

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

boost::asio::awaitable<capture_status> capture_session::open(const capture_option& in_option) {
  return impl_ptr_->open(in_option);
}

boost::asio::awaitable<capture_status> capture_session::start_record() { return impl_ptr_->start_record(); }

boost::asio::awaitable<capture_status> capture_session::stop() { return impl_ptr_->stop(); }

boost::asio::awaitable<capture_status> capture_session::close() { return impl_ptr_->close(); }

void capture_session::attach(std::shared_ptr<ws_outlet> in_outlet) { impl_ptr_->attach(std::move(in_outlet)); }

void capture_session::detach() { impl_ptr_->detach(); }

std::shared_ptr<ws_outlet> capture_session::outlet() const { return impl_ptr_->outlet(); }

capture_state capture_session::state() const { return impl_ptr_->state(); }

std::uint64_t capture_session::frames() const { return impl_ptr_->frames(); }

boost::uuids::uuid capture_session::session_id() const { return impl_ptr_->session_id(); }

FSys::path capture_session::session_dir() const { return impl_ptr_->session_dir(); }

const capture_session::executor_type& capture_session::get_executor() const { return impl_ptr_->get_executor(); }

}  // namespace doodle::face_capture
