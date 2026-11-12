//
// Created by TD on 2026-11-6.
//

#include "frame_filter.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/logger/logger.h>

#include <avcpp/ffmpeg.h>
#include <avcpp/filter.h>
#include <avcpp/filtercontext.h>
#include <avcpp/filtergraph.h>
#include <avcpp/frame.h>
#include <avcpp/timestamp.h>
#include <avcpp/buffersink.h>
#include <avcpp/buffersrc.h>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <libavfilter/buffersrc.h>

namespace doodle::face_capture {

namespace {

av::Rational in_time_base() { return av::Rational{1, 1'000'000}; }

av::Rational out_time_base() { return av::Rational{1, k_fps}; }

av::PixelFormat bgr_pixel_format() { return av::PixelFormat{AV_PIX_FMT_BGR24}; }

class frame_buffer {
 public:
  av::VideoFrame make_frame(const cv::Mat& in_bgr, std::int64_t in_pts_us) {
    auto l_size = static_cast<std::size_t>(in_bgr.total()) * in_bgr.elemSize();
    if (bytes_.size() < l_size) bytes_.resize(l_size);
    cv::Mat l_out{in_bgr.rows, in_bgr.cols, CV_8UC3, bytes_.data(), static_cast<std::size_t>(in_bgr.cols) * 3};
    in_bgr.copyTo(l_out);

    av::VideoFrame l_frame{bytes_.data(), l_size, bgr_pixel_format(), in_bgr.cols, in_bgr.rows, 1};
    l_frame.setTimeBase(in_time_base());
    l_frame.setPts(av::Timestamp{in_pts_us, in_time_base()});
    return l_frame;
  }

 private:
  std::vector<std::uint8_t> bytes_{};
};

}  // namespace

class frame_filter::impl {
 public:
  impl(const frame_filter_option& in_option) : option_(in_option) {}

  bool init() {
    DOODLE_CHICK(option_.width_ > 0 && option_.height_ > 0, "fps 滤镜尺寸非法: {}x{}", option_.width_, option_.height_);
    DOODLE_CHICK(!option_.graph_.empty(), "fps 滤镜描述为空");

    filter_ = av::Filter{"fps"};
    DOODLE_CHICK(static_cast<bool>(filter_), "当前 FFmpeg 构建缺少 fps 滤镜 (avfilter)");

    graph_.setAutoConvert(AVFILTER_AUTO_CONVERT_ALL);

    const auto l_buffer_args = fmt::format(
        "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect=1/1", option_.width_, option_.height_,
        static_cast<int>(AV_PIX_FMT_BGR24), in_time_base().getNumerator(), in_time_base().getDenominator()
    );

    buffersrc_ctx_  = graph_.createFilter(av::Filter{"buffer"}, "in", l_buffer_args);
    buffersink_ctx_ = graph_.createFilter(av::Filter{"buffersink"}, "out", "");

    filter_ctx_ = graph_.allocFilter(filter_, "fps");
    filter_ctx_.init(option_.graph_);

    buffersrc_ctx_.link(0, filter_ctx_, 0);
    filter_ctx_.link(0, buffersink_ctx_, 0);

    graph_.config();

    buffersrc_ = av::BufferSrcFilterContext{buffersrc_ctx_};
    buffersink_ = av::BufferSinkFilterContext{buffersink_ctx_};
    return true;
  }

  void push(const cv::Mat& in_bgr, std::int64_t in_pts_us) {
    DOODLE_CHICK(
        !in_bgr.empty() && in_bgr.cols == option_.width_ && in_bgr.rows == option_.height_,
        "fps 滤镜输入帧尺寸不符: {}x{}, 期望 {}x{}", in_bgr.cols, in_bgr.rows, option_.width_, option_.height_
    );

    auto l_frame = buffers_.make_frame(in_bgr, in_pts_us);
    buffersrc_.writeVideoFrame(l_frame);
    drain();
  }

  void flush() {
    (void)av_buffersrc_add_frame_flags(buffersrc_ctx_.raw(), nullptr, 0);
    drain();
  }

  std::uint64_t output_frames() const { return output_; }

  std::vector<filtered_frame> take_frames() { return std::move(pending_); }

 private:
  void drain() {
    av::VideoFrame l_frame{};
    while (buffersink_.getVideoFrame(l_frame)) pending_.push_back(to_frame(l_frame));
  }

  filtered_frame to_frame(av::VideoFrame& in_frame) {
    filtered_frame l_out{
        .mat_ = cv::Mat(in_frame.raw()->height, in_frame.raw()->width, CV_8UC3), .seq_ = output_++
    };
    in_frame.copyToBuffer(l_out.mat_.data, l_out.mat_.total() * 3, 1);
    return l_out;
  }

  frame_filter_option option_{};

  av::Filter filter_{};
  av::FilterGraph graph_{};
  av::FilterContext buffersrc_ctx_{};
  av::FilterContext buffersink_ctx_{};
  av::FilterContext filter_ctx_{};
  av::BufferSrcFilterContext buffersrc_{};
  av::BufferSinkFilterContext buffersink_{};

  frame_buffer buffers_{};
  std::vector<filtered_frame> pending_{};

  std::uint64_t output_{0};
};

frame_filter::frame_filter(const frame_filter_option& in_option) : impl_ptr_(std::make_unique<impl>(in_option)) {
  impl_ptr_->init();
}

frame_filter::frame_filter(frame_filter&&) noexcept = default;

frame_filter& frame_filter::operator=(frame_filter&&) noexcept = default;

frame_filter::~frame_filter() = default;

void frame_filter::push(const cv::Mat& in_bgr, std::int64_t in_pts_us) { impl_ptr_->push(in_bgr, in_pts_us); }

void frame_filter::flush() { impl_ptr_->flush(); }

std::vector<filtered_frame> frame_filter::take_frames() { return impl_ptr_->take_frames(); }

std::uint64_t frame_filter::output_frames() const { return impl_ptr_->output_frames(); }

}  // namespace doodle::face_capture
