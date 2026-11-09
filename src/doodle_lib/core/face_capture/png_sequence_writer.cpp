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
  explicit frame_spool(const FSys::path& in_dir) : dir_(in_dir) {}

  std::uint64_t written() const { return written_; }

  bool write(const cv::Mat& in_image) {
    std::vector<std::uint8_t> l_bytes{};
    if (!cv::imencode(".png", in_image, l_bytes, {cv::IMWRITE_PNG_COMPRESSION, k_png_compression})) return false;

    auto l_path = dir_ / png_file_name(written_);
    std::ofstream l_file{l_path, std::ios::binary | std::ios::trunc};
    if (!l_file) return false;
    l_file.write(reinterpret_cast<const char*>(l_bytes.data()), static_cast<std::streamsize>(l_bytes.size()));
    if (!l_file) return false;

    ++written_;
    return true;
  }

 private:
  FSys::path dir_{};
  std::uint64_t written_{0};
};

}  // namespace

FSys::path frames_dir_of(const FSys::path& in_session_dir) { return in_session_dir / "frames"; }

FSys::path png_tmp_dir_of(const FSys::path& in_session_dir) { return in_session_dir / "png_tmp"; }

png_sequence_result write_png_sequence(const png_sequence_option& in_option) {
  DOODLE_CHICK_HTTP(
      FSys::exists(in_option.video_), unprocessable_entity, "视频文件不存在: {}", in_option.video_.string()
  );

  align_record l_record{};
  std::string l_read_error{};
  try {
    l_record = read_align(in_option.session_dir_);
  } catch (const std::exception& in_err) {
    l_read_error = in_err.what();
  }

  DOODLE_CHICK_HTTP(l_read_error.empty(), unprocessable_entity, "读取 align.json 失败: {}", l_read_error);
  auto l_expect_raw_frames = l_record.raw_frames_ > 0 ? l_record.raw_frames_ : l_record.frames_.size();
  DOODLE_CHICK_HTTP(l_expect_raw_frames > 0, unprocessable_entity, "align.json 中没有帧记录");

  auto l_tmp_dir    = png_tmp_dir_of(in_option.session_dir_);
  auto l_frames_dir = frames_dir_of(in_option.session_dir_);
  if (FSys::exists(l_tmp_dir)) FSys::remove_all(l_tmp_dir);
  FSys::create_directories(l_tmp_dir);

  frame_spool l_spool{l_tmp_dir};
  std::int32_t l_width{0};
  std::int32_t l_height{0};

  {
    av::FormatContext l_format{};
    l_format.openInput(in_option.video_.string());
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
      DOODLE_CHICK_HTTP(false, unprocessable_entity, "视频文件中没有视频流");
    }

    auto l_codec = l_stream.codecParameters().decodingCodec();
    if (l_codec.isNull() || !l_codec.isDecoder()) {
      FSys::remove_all(l_tmp_dir);
      DOODLE_CHICK_HTTP(false, unprocessable_entity, "视频流没有可用的解码器");
    }

    av::VideoDecoderContext l_decoder{l_stream, l_codec};
    l_decoder.open();

    const int l_src_width  = l_decoder.width();
    const int l_src_height = l_decoder.height();
    auto l_src_pix_fmt     = l_decoder.pixelFormat();
    if (l_src_width <= 0 || l_src_height <= 0) {
      FSys::remove_all(l_tmp_dir);
      DOODLE_CHICK_HTTP(false, unprocessable_entity, "视频尺寸非法");
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
      DOODLE_CHICK_HTTP(false, unprocessable_entity, "缩放后尺寸非法");
    }

    av::VideoRescaler l_rescaler{l_width, l_height, rgb_pixel_format(), l_src_width, l_src_height, l_src_pix_fmt};
    decoded_frame_buffer l_buffer{};

    auto l_store = [&](av::VideoFrame in_frame) {
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

  if (l_spool.written() == 0) {
    FSys::remove_all(l_tmp_dir);
    DOODLE_CHICK_HTTP(false, unprocessable_entity, "视频中没有可解码的帧");
  }

  if (FSys::exists(l_frames_dir)) FSys::remove_all(l_frames_dir);
  FSys::rename(l_tmp_dir, l_frames_dir);

  if (l_expect_raw_frames != l_spool.written())
    SPDLOG_WARN(
        "视频解码帧数 {} 与 align.json 记录 {} 不一致", l_spool.written(), l_expect_raw_frames
    );

  png_sequence_result l_result{};
  l_result.frame_count_ = l_spool.written();
  l_result.width_       = l_width;
  l_result.height_      = l_height;
  l_result.frames_dir_  = l_frames_dir;
  return l_result;
}

}  // namespace doodle::face_capture
