//
// Created by TD on 2026-11-6.
//

#include "png_sequence_writer.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/face_capture/frame_align.h>
#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/logger/logger.h>

#include <avcpp/codec.h>
#include <avcpp/codeccontext.h>
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
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace doodle::face_capture {

namespace {

using clock_type = std::chrono::steady_clock;

constexpr std::int32_t k_png_compression{3};

std::string png_file_name(std::uint64_t in_seq) { return fmt::format("frame_{:0{}d}.png", in_seq, k_png_digits); }

av::PixelFormat rgb_pixel_format() { return av::PixelFormat{AV_PIX_FMT_RGB24}; }

std::int32_t even(std::int32_t in_value) { return std::max<std::int32_t>(in_value - in_value % 2, 2); }

png_sequence_result fail(error_code_t in_code, std::string in_message) {
  png_sequence_result l_result{};
  l_result.error_code_ = in_code;
  l_result.message_    = std::move(in_message);
  return l_result;
}

class decoded_frame_buffer {
 public:
  cv::Mat wrap(av::VideoFrame in_frame, int in_width, int in_height) {
    auto l_size = static_cast<std::size_t>(in_width) * in_height * 3;
    if (bytes_.size() < l_size) bytes_.resize(l_size);
    in_frame.copyToBuffer(bytes_.data(), l_size, 1);
    return cv::Mat{in_height, in_width, CV_8UC3, bytes_.data(), static_cast<std::size_t>(in_width) * 3}.clone();
  }

 private:
  std::vector<std::uint8_t> bytes_{};
};

class frame_spool {
 public:
  frame_spool(const FSys::path& in_dir, const align_record& in_record) : dir_(in_dir), record_(in_record) {}

  bool done() const { return index_ >= record_.frames_.size(); }

  std::uint64_t written() const { return written_; }

  const cv::Mat& last() const { return last_; }

  bool write(const cv::Mat& in_image) { return write_one(in_image); }

  bool write_gap_frame() { return write_one(last_); }

  bool catch_up(std::uint64_t in_seq) {
    while (!done() && record_.frames_[index_].seq_ < in_seq) {
      if (last_.empty()) return false;
      if (!write_one(last_)) return false;
    }
    return !done() && record_.frames_[index_].seq_ == in_seq;
  }
 private:
  bool write_one(const cv::Mat& in_image) {
    if (index_ >= record_.frames_.size()) return false;
    std::vector<std::uint8_t> l_bytes{};
    if (!cv::imencode(".png", in_image, l_bytes, {cv::IMWRITE_PNG_COMPRESSION, k_png_compression})) return false;

    auto l_path = dir_ / png_file_name(record_.frames_[index_].seq_);
    std::ofstream l_file{l_path, std::ios::binary | std::ios::trunc};
    if (!l_file) return false;
    l_file.write(reinterpret_cast<const char*>(l_bytes.data()), static_cast<std::streamsize>(l_bytes.size()));
    if (!l_file) return false;

    last_ = in_image;
    ++index_;
    ++written_;
    return true;
  }

  FSys::path dir_{};
  align_record record_{};
  cv::Mat last_{};
  std::size_t index_{0};
  std::uint64_t written_{0};
};

}  // namespace

FSys::path frames_dir_of(const FSys::path& in_session_dir) { return in_session_dir / "frames"; }

FSys::path png_tmp_dir_of(const FSys::path& in_session_dir) { return in_session_dir / "png_tmp"; }

png_sequence_result write_png_sequence(const png_sequence_option& in_option) {
  if (!FSys::exists(in_option.raw_mkv_))
    return fail(error_code_t::raw_mkv_missing, fmt::format("raw.mkv 不存在: {}", in_option.raw_mkv_.string()));

  align_record l_record{};
  try {
    l_record = read_align(in_option.session_dir_);
  } catch (const std::exception& in_err) {
    return fail(error_code_t::png_sequence_invalid, fmt::format("读取 align.json 失败: {}", in_err.what()));
  }

  auto l_expect = expect_frame_count(l_record);
  if (l_expect == 0) return fail(error_code_t::png_sequence_invalid, "align.json 中没有帧记录");

  auto l_tmp_dir    = png_tmp_dir_of(in_option.session_dir_);
  auto l_frames_dir = frames_dir_of(in_option.session_dir_);
  if (FSys::exists(l_tmp_dir)) FSys::remove_all(l_tmp_dir);
  FSys::create_directories(l_tmp_dir);

  frame_spool l_spool{l_tmp_dir, l_record};
  std::int32_t l_width{0};
  std::int32_t l_height{0};

  {
    av::FormatContext l_format{};
    l_format.openInput(in_option.raw_mkv_.string());
    l_format.findStreamInfo();

    av::Stream l_stream{};
    for (std::size_t l_index = 0; l_index < l_format.streamsCount(); ++l_index) {
      auto l_candidate = l_format.stream(l_index);
      if (!l_candidate.isVideo()) continue;
      l_stream = l_candidate;
      break;
    }
    if (!l_stream.isValid()) {
      FSys::remove_all(l_tmp_dir);
      return fail(error_code_t::raw_mkv_missing, "raw.mkv 中没有视频流");
    }

    auto l_codec = l_stream.codecParameters().decodingCodec();
    if (l_codec.isNull() || !l_codec.isDecoder()) {
      FSys::remove_all(l_tmp_dir);
      return fail(error_code_t::raw_mkv_missing, "raw.mkv 视频流没有可用的解码器");
    }

    av::VideoDecoderContext l_decoder{l_stream, l_codec};
    l_decoder.open();

    const int l_src_width  = l_decoder.width();
    const int l_src_height = l_decoder.height();
    auto l_src_pix_fmt     = l_decoder.pixelFormat();
    if (l_src_width <= 0 || l_src_height <= 0) {
      FSys::remove_all(l_tmp_dir);
      return fail(error_code_t::raw_mkv_missing, "raw.mkv 视频尺寸非法");
    }

    auto l_max_width = in_option.decode_max_width_ > 0 ? in_option.decode_max_width_ : k_default_width;
    l_width          = even(l_src_width);
    l_height         = even(l_src_height);
    if (l_width > l_max_width) {
      l_width  = even(l_max_width);
      l_height = even(static_cast<std::int32_t>(
          static_cast<double>(l_src_height) * static_cast<double>(l_max_width) / static_cast<double>(l_src_width)
      ));
    }
    if (l_width <= 0 || l_height <= 0) {
      FSys::remove_all(l_tmp_dir);
      return fail(error_code_t::raw_mkv_missing, "raw.mkv 缩放后尺寸非法");
    }

    av::VideoRescaler l_rescaler{l_width, l_height, rgb_pixel_format(), l_src_width, l_src_height, l_src_pix_fmt};
    decoded_frame_buffer l_buffer{};

    auto l_store = [&](av::VideoFrame in_frame) {
      if (l_spool.done()) return;
      if (!l_spool.catch_up(l_spool.written())) return;
      auto l_out = l_rescaler.rescale(in_frame);
      if (!l_out.isValid()) return;
      l_spool.write(l_buffer.wrap(l_out, l_width, l_height));
    };

    while (auto l_packet = l_format.readPacket()) {
      if (l_packet.streamIndex() != l_stream.index()) continue;
      auto l_frame = l_decoder.decode(l_packet);
      if (l_frame) l_store(l_frame);
    }
    while (auto l_frame = l_decoder.decode({})) l_store(l_frame);

    l_decoder.close();
    l_format.close();
  }

  while (!l_spool.done()) {
    if (l_spool.last().empty()) break;
    if (!l_spool.write(l_spool.last())) break;
  }

  if (l_spool.done()) {
    if (FSys::exists(l_frames_dir)) FSys::remove_all(l_frames_dir);
    FSys::rename(l_tmp_dir, l_frames_dir);

    png_sequence_result l_result{};
    l_result.ok_          = true;
    l_result.frame_count_ = l_expect;
    l_result.width_       = l_width;
    l_result.height_      = l_height;
    l_result.frames_dir_  = l_frames_dir;
    return l_result;
  }

  return fail(
      error_code_t::png_sequence_invalid,
      fmt::format("PNG 序列生成不完整: {}/{}", l_spool.written(), l_expect)
  );
}

}  // namespace doodle::face_capture
