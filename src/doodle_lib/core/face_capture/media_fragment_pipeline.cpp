//
// Created by TD on 2026-11-6.
//

#include "media_fragment_pipeline.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/face_capture/face_capture_types.h>
#include <doodle_lib/logger/logger.h>

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

#include <algorithm>
#include <atomic>
#include <avcpp/codec.h>
#include <avcpp/codeccontext.h>
#include <avcpp/dictionary.h>
#include <avcpp/ffmpeg.h>
#include <avcpp/format.h>
#include <avcpp/formatcontext.h>
#include <avcpp/frame.h>
#include <avcpp/packet.h>
#include <avcpp/rational.h>
#include <avcpp/stream.h>
#include <avcpp/timestamp.h>
#include <avcpp/videorescaler.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace doodle::face_capture {

namespace {

av::PixelFormat bgr_pixel_format() { return av::PixelFormat{AV_PIX_FMT_BGR24}; }

av::PixelFormat yuv_pixel_format() { return av::PixelFormat{AV_PIX_FMT_YUV420P}; }

av::Rational frame_time_base() { return av::Rational{1, k_fps}; }

av::Rational frame_rate() { return av::Rational{k_fps, 1}; }

av::Rational raw_time_base() { return av::Rational{1, 1000}; }

std::int32_t even(std::int32_t in_value) { return in_value - in_value % 2; }

class frame_buffer {
 public:
  void resize(const cv::Mat& in_bgr) {
    auto l_size = static_cast<std::size_t>(in_bgr.total()) * in_bgr.elemSize();
    if (bytes_.size() < l_size) bytes_.resize(l_size);
  }

  cv::Mat wrap(const cv::Mat& in_bgr) {
    resize(in_bgr);
    cv::Mat l_out{in_bgr.rows, in_bgr.cols, CV_8UC3, bytes_.data(), static_cast<std::size_t>(in_bgr.cols) * 3};
    in_bgr.copyTo(l_out);
    return l_out;
  }

  av::VideoFrame make_frame(const cv::Mat& in_bgr) {
    auto l_clone = wrap(in_bgr);
    return av::VideoFrame{
        l_clone.data, static_cast<std::size_t>(l_clone.total()) * l_clone.elemSize(), bgr_pixel_format(),
        l_clone.cols, l_clone.rows, l_clone.cols * 3
    };
  }

 private:
  std::vector<std::uint8_t> bytes_{};
};

void configure_encoder(
    av::VideoEncoderContext& in_ctx, std::int32_t in_width, std::int32_t in_height, bool in_low_latency
) {
  in_ctx.setWidth(in_width);
  in_ctx.setHeight(in_height);
  in_ctx.setTimeBase(frame_time_base());
  in_ctx.setGopSize(k_keyint);
  in_ctx.setMaxBFrames(0);
  in_ctx.setPixelFormat(yuv_pixel_format());
  in_ctx.setOption("preset", "veryfast");
  in_ctx.setOption("crf", "23", AV_OPT_SEARCH_CHILDREN);
  in_ctx.setOption("sc_threshold", "0");
  in_ctx.setOption("profile", "main");
  if (in_low_latency) in_ctx.setOption("tune", "zerolatency");
}

av::Timestamp frame_timestamp(std::uint64_t in_seq) {
  return av::Timestamp{static_cast<std::int64_t>(in_seq), frame_time_base()};
}

av::Timestamp raw_timestamp(std::int64_t in_wall_ms) { return av::Timestamp{in_wall_ms, raw_time_base()}; }

}  // namespace

class h264_fragmenter::impl {
 public:
  impl(std::int32_t in_width, std::int32_t in_height)
      : width_(even(in_width)),
        height_(even(in_height)),
        codec_(av::findEncodingCodec(std::string{"libx264"})),
        enc_ctx_(codec_) {}

  bool init() {
    DOODLE_CHICK(codec_.isEncoder(), "找不到 libx264 编码器");
    DOODLE_CHICK(width_ > 0 && height_ > 0, "推流编码器尺寸非法 {}x{}", width_, height_);

    io_ = std::make_unique<memory_io>();

    configure_encoder(enc_ctx_, width_, height_, true);
    enc_ctx_.setOption("x264-params", fmt::format("keyint={}:min-keyint={}:scenecut=0", k_keyint, k_keyint));
    enc_ctx_.open();
    DOODLE_CHICK(enc_ctx_.isOpened(), "打开 libx264 编码器失败");

    format_context_.setFormat(av::OutputFormat{std::string{"mp4"}, std::string{}, std::string{}});
    format_context_.openOutput(io_.get());
    stream_ = format_context_.addStream(enc_ctx_);
    stream_.setTimeBase(frame_time_base());
    stream_.setFrameRate(frame_rate());
    stream_.setAverageFrameRate(frame_rate());

    av::Dictionary l_opts{};
    l_opts.set("movflags", "frag_keyframe+empty_moov+default_base_moof");
    l_opts.set("flush_packets", "1");
    l_opts.set("mov_write_trailer", "0");
    format_context_.writeHeader(l_opts);

    rescaler_ = av::VideoRescaler{
        width_, height_, yuv_pixel_format(), width_, height_, bgr_pixel_format(), av::SwsFlagBicubic
    };

    pending_.emplace_back();
    pending_.back().data_      = io_->take();
    pending_.back().seq_begin_ = 0;
    pending_.back().seq_end_   = 0;
    pending_.back().key_       = true;
    init_segment_              = pending_.back().data_;

    fragment_open_   = true;
    open_seq_begin_  = 0;
    return true;
  }

  bool push(const cv::Mat& in_bgr, std::uint64_t in_seq) {
    if (!enc_ctx_.isOpened()) return false;
    if (in_bgr.empty() || in_bgr.cols != width_ || in_bgr.rows != height_ || in_bgr.type() != CV_8UC3)
      return false;

    auto l_src = buffers_.make_frame(in_bgr);
    auto l_yuv = rescaler_.rescale(l_src);
    l_yuv.setTimeBase(frame_time_base());
    l_yuv.setPts(frame_timestamp(in_seq));
    seq_ = in_seq + 1;

    auto l_packet = enc_ctx_.encode(l_yuv);
    if (l_packet) write_packet(l_packet);
    return true;
  }

  bool finish() {
    if (!enc_ctx_.isOpened()) return true;
    try {
      flush();
    } catch (...) {
    }
    enc_ctx_ = av::VideoEncoderContext{};
    flush_open_fragment();
    return true;
  }

  std::vector<fragment_t> take_fragments() {
    flush_open_fragment();
    return std::move(pending_);
  }

  const std::vector<std::uint8_t>& init_segment() const { return init_segment_; }

  std::uint64_t frames() const { return seq_; }

 private:
  void flush() {
    while (true) {
      av::Packet l_packet = enc_ctx_.encode();
      if (!l_packet) break;
      write_packet(l_packet);
    }
  }
  class memory_io : public av::CustomIO {
   public:
    int write(const std::uint8_t* in_data, std::size_t in_size) override {
      buffer_.insert(buffer_.end(), in_data, in_data + in_size);
      return static_cast<int>(in_size);
    }

    std::vector<std::uint8_t> take() {
      auto l_out = std::move(buffer_);
      buffer_.clear();
      return l_out;
    }

    const char* name() const override { return "face_capture_frag"; }
    int seekable() const override { return 0; }

   private:
    std::vector<std::uint8_t> buffer_{};
  };

  void write_packet(const av::Packet& in_packet) {
    format_context_.writePacket(in_packet);
    if ((in_packet.flags() & AV_PKT_FLAG_KEY) != 0) {
      flush_open_fragment();
      open_seq_begin_ = seq_ > 0 ? seq_ - 1 : 0;
      fragment_open_  = true;
    }
  }

  void flush_open_fragment() {
    if (!fragment_open_) return;
    auto l_bytes = io_->take();
    if (l_bytes.empty()) return;
    fragment_t l_fragment{};
    l_fragment.data_      = std::move(l_bytes);
    l_fragment.seq_begin_ = open_seq_begin_;
    l_fragment.seq_end_   = seq_ > 0 ? seq_ - 1 : 0;
    l_fragment.key_       = true;
    pending_.emplace_back(std::move(l_fragment));
    fragment_open_ = false;
  }

  std::int32_t width_{0};
  std::int32_t height_{0};
  av::Codec codec_{};
  av::FormatContext format_context_{};
  av::VideoEncoderContext enc_ctx_{};
  av::Stream stream_{};
  av::VideoRescaler rescaler_{};
  std::unique_ptr<memory_io> io_{};
  frame_buffer buffers_{};
  std::uint64_t seq_{0};
  std::vector<std::uint8_t> init_segment_{};
  std::uint64_t open_seq_begin_{0};
  bool fragment_open_{false};
  std::vector<fragment_t> pending_{};
};

h264_fragmenter::h264_fragmenter(std::int32_t in_width, std::int32_t in_height)
    : impl_ptr_(std::make_unique<impl>(in_width, in_height)) {}

h264_fragmenter::h264_fragmenter(h264_fragmenter&&) noexcept = default;

h264_fragmenter& h264_fragmenter::operator=(h264_fragmenter&&) noexcept = default;

h264_fragmenter::~h264_fragmenter() = default;

bool h264_fragmenter::init() { return impl_ptr_->init(); }

bool h264_fragmenter::push(const cv::Mat& in_bgr, std::uint64_t in_seq) { return impl_ptr_->push(in_bgr, in_seq); }

std::uint64_t h264_fragmenter::frames() const { return impl_ptr_ ? impl_ptr_->frames() : 0; }

bool h264_fragmenter::finish() { return impl_ptr_ ? impl_ptr_->finish() : true; }

const std::vector<std::uint8_t>& h264_fragmenter::init_segment() const { return impl_ptr_->init_segment(); }

std::vector<fragment_t> h264_fragmenter::take_fragments() { return impl_ptr_->take_fragments(); }

class mp4_writer::impl {
 public:  impl(const FSys::path& in_path, std::int32_t in_width, std::int32_t in_height, bool in_low_latency)
      : path_(in_path),
        width_(even(in_width)),
        height_(even(in_height)),
        low_latency_(in_low_latency),
        codec_(av::findEncodingCodec(std::string{"libx264"})) {}

  bool init() {
    DOODLE_CHICK(codec_.isEncoder(), "找不到 libx264 编码器");
    DOODLE_CHICK(width_ > 0 && height_ > 0, "mp4 编码器尺寸非法 {}x{}", width_, height_);
    if (path_.has_parent_path() && !FSys::exists(path_.parent_path()))
      FSys::create_directories(path_.parent_path());

    configure_encoder(enc_ctx_, width_, height_, low_latency_);
    enc_ctx_.open();
    DOODLE_CHICK(enc_ctx_.isOpened(), "打开 libx264 编码器失败");

    format_context_.openOutput(path_.string());
    stream_ = format_context_.addStream(enc_ctx_);
    stream_.setTimeBase(frame_time_base());
    stream_.setFrameRate(frame_rate());
    stream_.setAverageFrameRate(frame_rate());
    format_context_.writeHeader();

    rescaler_ = av::VideoRescaler{
        width_, height_, yuv_pixel_format(), width_, height_, bgr_pixel_format(), av::SwsFlagBicubic
    };
    return true;
  }

  bool push(const cv::Mat& in_bgr, std::uint64_t in_seq) {
    if (!enc_ctx_.isOpened()) return false;
    if (in_bgr.empty() || in_bgr.cols != width_ || in_bgr.rows != height_ || in_bgr.type() != CV_8UC3)
      return false;

    auto l_src = buffers_.make_frame(in_bgr);
    auto l_yuv = rescaler_.rescale(l_src);
    l_yuv.setTimeBase(frame_time_base());
    l_yuv.setPts(frame_timestamp(in_seq));
    seq_ = in_seq + 1;

    auto l_packet = enc_ctx_.encode(l_yuv);
    if (l_packet) format_context_.writePacket(l_packet);
    return true;
  }

  bool finish() {
    if (!enc_ctx_.isOpened()) return true;
    try {
      flush();
    } catch (...) {
    }
    enc_ctx_ = av::VideoEncoderContext{};
    try {
      format_context_.writeTrailer();
      format_context_.close();
    } catch (...) {
    }
    return true;
  }

  std::uint64_t frames() const { return seq_; }

 private:
  void write_packet(const av::Packet& in_packet) { format_context_.writePacket(in_packet); }

  void flush() {
    while (true) {
      av::Packet l_packet = enc_ctx_.encode();
      if (!l_packet) break;
      write_packet(l_packet);
    }
  }

  FSys::path path_{};
  std::int32_t width_{0};
  std::int32_t height_{0};
  bool low_latency_{true};
  av::Codec codec_{};
  av::FormatContext format_context_{};
  av::VideoEncoderContext enc_ctx_{};
  av::Stream stream_{};
  av::VideoRescaler rescaler_{};
  frame_buffer buffers_{};
  std::uint64_t seq_{0};
};

mp4_writer::mp4_writer(const FSys::path& in_path, std::int32_t in_width, std::int32_t in_height, bool in_low_latency)
    : impl_ptr_(std::make_unique<impl>(in_path, in_width, in_height, in_low_latency)) {}

mp4_writer::mp4_writer(mp4_writer&&) noexcept = default;

mp4_writer& mp4_writer::operator=(mp4_writer&&) noexcept = default;

mp4_writer::~mp4_writer() = default;

bool mp4_writer::init() { return impl_ptr_->init(); }

bool mp4_writer::push(const cv::Mat& in_bgr, std::uint64_t in_seq) { return impl_ptr_->push(in_bgr, in_seq); }

std::uint64_t mp4_writer::frames() const { return impl_ptr_ ? impl_ptr_->frames() : 0; }

bool mp4_writer::finish() { return impl_ptr_ ? impl_ptr_->finish() : true; }

class raw_sink_thread {
 public:
  raw_sink_thread(const FSys::path& in_path, std::int32_t in_width, std::int32_t in_height)
      : path_(in_path), width_(even(in_width)), height_(even(in_height)) {}

  ~raw_sink_thread() { stop(); }

  bool start() {
    codec_ = av::findEncodingCodec(AV_CODEC_ID_FFV1);
    DOODLE_CHICK(codec_.isEncoder(), "找不到 FFV1 编码器");
    DOODLE_CHICK(width_ > 0 && height_ > 0, "raw.mkv 编码器尺寸非法 {}x{}", width_, height_);
    if (path_.has_parent_path() && !FSys::exists(path_.parent_path()))
      FSys::create_directories(path_.parent_path());

    enc_ctx_.setWidth(width_);
    enc_ctx_.setHeight(height_);
    enc_ctx_.setTimeBase(raw_time_base());
    enc_ctx_.setPixelFormat(yuv_pixel_format());
    enc_ctx_.setOption("level", "3");
    enc_ctx_.open();
    DOODLE_CHICK(enc_ctx_.isOpened(), "打开 FFV1 编码器失败");

    format_context_.openOutput(path_.string());
    stream_ = format_context_.addStream(enc_ctx_);
    stream_.setTimeBase(raw_time_base());
    stream_.setFrameRate(av::Rational{0, 1});
    format_context_.writeHeader();

    rescaler_ = av::VideoRescaler{
        width_, height_, yuv_pixel_format(), width_, height_, bgr_pixel_format(), av::SwsFlagBicubic
    };

    worker_ = std::thread{[this] { run(); }};
    return true;
  }

  bool push(const cv::Mat& in_bgr, std::int64_t in_wall_ms) {
    if (in_bgr.empty() || in_bgr.cols != width_ || in_bgr.rows != height_ || in_bgr.type() != CV_8UC3)
      return false;
    {
      std::unique_lock l_lock{mutex_};
      if (stopping_ || closed_) return false;
      if (queue_.size() >= static_cast<std::size_t>(k_max_raw_queue_frames)) {
        SPDLOG_WARN("raw.mkv 写盘队列已满({}), 丢弃当前帧", k_max_raw_queue_frames);
        return false;
      }
      queue_.push_back(queued_frame{.mat_ = in_bgr.clone(), .wall_ms_ = in_wall_ms});
    }
    cv_.notify_one();
    return true;
  }

  bool finish() {
    {
      std::unique_lock l_lock{mutex_};
      stopping_ = true;
    }
    cv_.notify_all();

    auto l_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds{k_stop_drain_timeout_ms};
    {
      std::unique_lock l_lock{mutex_};
      cv_.wait_until(l_lock, l_deadline, [this] { return queue_.empty() || error_; });
      if (!queue_.empty()) {
        SPDLOG_WARN("raw.mkv 写盘超时, 丢弃 {} 帧", queue_.size());
        queue_.clear();
      }
      closed_ = true;
    }
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();

    if (error_) SPDLOG_ERROR("raw.mkv 写盘线程异常, 尾部可能不完整");
    if (enc_ctx_.isOpened()) {
      flush_encoder();
      enc_ctx_ = av::VideoEncoderContext{};
      format_context_.writeTrailer();
      format_context_.close();
    }
    return !error_;
  }

  std::uint64_t written() const { return written_.load(); }

 private:
  struct queued_frame {
    cv::Mat mat_{};
    std::int64_t wall_ms_{0};
  };

  void run() {
    while (true) {
      queued_frame l_frame{};
      {
        std::unique_lock l_lock{mutex_};
        cv_.wait(l_lock, [this] { return !queue_.empty() || closed_; });
        if (queue_.empty()) break;
        l_frame = std::move(queue_.front());
        queue_.pop_front();
      }
      encode_frame(l_frame);
      cv_.notify_all();
    }
  }

  void encode_frame(const queued_frame& in_frame) {
    try {
      auto l_src = buffers_.make_frame(in_frame.mat_);
      auto l_yuv = rescaler_.rescale(l_src);
      l_yuv.setTimeBase(raw_time_base());
      l_yuv.setPts(raw_timestamp(in_frame.wall_ms_));
      auto l_packet = enc_ctx_.encode(l_yuv);
      if (l_packet) format_context_.writePacket(l_packet);
      ++written_;
    } catch (...) {
      std::unique_lock l_lock{mutex_};
      error_ = true;
    }
  }

  void flush_encoder() {
    while (true) {
      av::Packet l_packet = enc_ctx_.encode();
      if (!l_packet) break;
      format_context_.writePacket(l_packet);
    }
  }

  void stop() {
    if (worker_.joinable()) {
      {
        std::unique_lock l_lock{mutex_};
        closed_   = true;
        stopping_ = true;
      }
      cv_.notify_all();
      worker_.join();
    }
    if (enc_ctx_.isOpened()) {
      try {
        flush_encoder();
        enc_ctx_ = av::VideoEncoderContext{};
        format_context_.writeTrailer();
        format_context_.close();
      } catch (...) {
      }
    }
  }

  FSys::path path_{};
  std::int32_t width_{0};
  std::int32_t height_{0};

  std::thread worker_{};
  std::mutex mutex_{};
  std::condition_variable cv_{};
  std::deque<queued_frame> queue_{};
  bool stopping_{false};
  bool closed_{false};
  bool error_{false};

  av::Codec codec_{};
  av::FormatContext format_context_{};
  av::VideoEncoderContext enc_ctx_{};
  av::Stream stream_{};
  av::VideoRescaler rescaler_{};
  frame_buffer buffers_{};
  std::atomic<std::uint64_t> written_{0};
};

class ffv1_raw_writer : public raw_writer {
 public:
  ffv1_raw_writer(const FSys::path& in_path, std::int32_t in_width, std::int32_t in_height)
      : sink_(std::make_shared<raw_sink_thread>(in_path, in_width, in_height)) {
    sink_->start();
  }

  bool push(const cv::Mat& in_bgr, std::int64_t in_wall_ms) override { return sink_->push(in_bgr, in_wall_ms); }

  std::uint64_t frames() const override { return sink_->written(); }

  bool finish() override { return sink_->finish(); }

 private:
  std::shared_ptr<raw_sink_thread> sink_{};
};

std::shared_ptr<raw_writer> raw_writer::make(
    const FSys::path& in_path, std::int32_t in_width, std::int32_t in_height
) {
  return std::make_shared<ffv1_raw_writer>(in_path, in_width, in_height);
}

}  // namespace doodle::face_capture
