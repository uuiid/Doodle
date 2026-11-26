//
// Created by TD on 2026-11-10.
//

#include "face_capture_transcode.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/face_capture/capture_session.h>
#include <doodle_lib/core/face_capture/face_capture_types.h>
#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/exe_warp/ue_exe.h>

#include <avcpp/audioresampler.h>
#include <avcpp/avconfig.h>
#include <avcpp/buffersink.h>
#include <avcpp/buffersrc.h>
#include <avcpp/codec.h>
#include <avcpp/codeccontext.h>
#include <avcpp/ffmpeg.h>
#include <avcpp/filter.h>
#include <avcpp/filtercontext.h>
#include <avcpp/filtergraph.h>
#include <avcpp/format.h>
#include <avcpp/formatcontext.h>
#include <avcpp/frame.h>
#include <avcpp/stream.h>
#include <avcpp/timestamp.h>
#include <avcpp/videorescaler.h>

#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <string>
#include <vector>

namespace doodle {

namespace {

constexpr std::string_view k_curves_file_name{"face_curves.dhfc"};

constexpr std::string_view k_log_file_name{"face_capture_log.json"};

constexpr std::int32_t k_png_compression{3};

constexpr std::uint32_t k_audio_sample_rate{16000};

constexpr std::uint16_t k_audio_channels{1};

constexpr std::uint16_t k_audio_bits{16};

FSys::path session_dir_of(const uuid& in_session_id) {
  return face_capture::capture_session::default_root() / fmt::format("{}", in_session_id);
}

FSys::path resolve_video(const FSys::path& in_session_dir) {
  for (const auto& l_name : {"take.mp4", "take.webm"}) {
    auto l_path = in_session_dir / l_name;
    if (FSys::exists(l_path)) return l_path;
  }
  return {};
}

bool write_png_frame(const FSys::path& in_dir, std::int32_t in_index, const cv::Mat& in_image) {
  auto l_name = fmt::format("frame_{:0{}d}.png", in_index, face_capture::k_png_digits);
  return cv::imwrite((in_dir / l_name).string(), in_image, {cv::IMWRITE_PNG_COMPRESSION, k_png_compression});
}

class fps_resampler {
 public:
  explicit fps_resampler(const av::VideoDecoderContext& in_dec_ctx) {
    auto l_time_base     = in_dec_ctx.timeBase();
    auto l_sample_aspect = in_dec_ctx.sampleAspectRatio();
    DOODLE_CHICK(l_sample_aspect != av::Rational{}, "视频的采样宽高比无效");

    const std::string l_buffer_args = fmt::format(
        "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}", in_dec_ctx.width(), in_dec_ctx.height(),
        static_cast<std::int32_t>(in_dec_ctx.pixelFormat()), l_time_base.getNumerator(),
        l_time_base.getDenominator(), l_sample_aspect.getNumerator(), l_sample_aspect.getDenominator()
    );
    graph_.setAutoConvert(AVFILTER_AUTO_CONVERT_ALL);

    buffersrc_ctx_  = graph_.createFilter(av::Filter{"buffer"}, "in", l_buffer_args);
    buffersink_ctx_ = graph_.createFilter(av::Filter{"buffersink"}, "out", "");

    const av::Filter l_fps_filter{"fps"};
    DOODLE_CHICK(l_fps_filter, "找不到 ffmpeg 滤镜 fps");
    auto l_fps_ctx = graph_.allocFilter(l_fps_filter, "fps");
    l_fps_ctx.init(fmt::format("fps={}", face_capture::k_fps));

    buffersrc_ctx_.link(0, l_fps_ctx, 0);
    l_fps_ctx.link(0, buffersink_ctx_, 0);

    graph_.config();
    buffersrc_  = av::BufferSrcFilterContext{buffersrc_ctx_};
    buffersink_ = av::BufferSinkFilterContext{buffersink_ctx_};
    rescaler_   = av::VideoRescaler{in_dec_ctx.width(), in_dec_ctx.height(), av::PixelFormat{AV_PIX_FMT_BGR24}};
  }

  void push(const av::VideoFrame& in_frame) { buffersrc_.writeVideoFrame(in_frame); }

  void flush() { (void)av_buffersrc_add_frame_flags(buffersrc_ctx_.raw(), nullptr, 0); }

  cv::Mat read_bgr() {
    av::VideoFrame l_frame{};
    if (!buffersink_.getVideoFrame(l_frame) || !l_frame || !l_frame.isValid()) return {};

    av::VideoFrame l_bgr{av::PixelFormat{AV_PIX_FMT_BGR24}, l_frame.width(), l_frame.height()};
    rescaler_.rescale(l_bgr, l_frame);

    cv::Mat l_image{
        l_bgr.height(), l_bgr.width(), CV_8UC3, l_bgr.data(0), static_cast<std::size_t>(l_bgr.raw()->linesize[0])
    };
    return l_image.clone();
  }

 private:
  av::FilterGraph graph_{};
  av::FilterContext buffersrc_ctx_{};
  av::BufferSrcFilterContext buffersrc_{};
  av::FilterContext buffersink_ctx_{};
  av::BufferSinkFilterContext buffersink_{};
  av::VideoRescaler rescaler_{};
};

std::int32_t decode_to_png_and_wav(
    const FSys::path& in_video, const FSys::path& in_frames_dir, const FSys::path& in_wav,
    std::int32_t in_target_fps
) {
  DOODLE_CHICK(in_target_fps > 0, "目标帧率必须大于 0");

  av::FormatContext l_format_ctx{};
  l_format_ctx.openInput(in_video.string());
  l_format_ctx.findStreamInfo();

  av::Stream l_video_stream{};
  av::Stream l_audio_stream{};
  for (std::size_t l_i = 0; l_i < l_format_ctx.streamsCount(); ++l_i) {
    auto l_stream = l_format_ctx.stream(l_i);
    if (l_stream.isVideo() && !l_video_stream.isValid())
      l_video_stream = l_stream;
    else if (l_stream.isAudio() && !l_audio_stream.isValid())
      l_audio_stream = l_stream;
  }
  DOODLE_CHICK(l_video_stream.isValid(), "视频 {} 没有视频流", in_video.generic_string());
  DOODLE_CHICK(l_audio_stream.isValid(), "视频 {} 没有音频流, 无法解算舌头", in_video.generic_string());

  av::Codec l_video_codec = l_video_stream.codecParameters().decodingCodec();
  DOODLE_CHICK(!l_video_codec.isNull(), "视频 {} 找不到解码器", in_video.generic_string());
  DOODLE_CHICK(l_video_codec.isDecoder(), "视频 {} 的编解码器不是解码器", in_video.generic_string());

  av::VideoDecoderContext l_dec_ctx{l_video_stream, l_video_codec};
  l_dec_ctx.open();

  av::AudioDecoderContext l_audio_dec_ctx{l_audio_stream};
  l_audio_dec_ctx.open();

  fps_resampler l_resampler{l_dec_ctx};
  std::int32_t l_count = 0;

  auto l_write_pending = [&]() {
    for (auto l_image = l_resampler.read_bgr(); !l_image.empty(); l_image = l_resampler.read_bgr()) {
      DOODLE_CHICK(
          write_png_frame(in_frames_dir, l_count, l_image), "写出第 {} 帧 png 失败, 目录 {}", l_count,
          in_frames_dir.generic_string()
      );
      ++l_count;
    }
  };

  av::OutputFormat l_out_format{"wav"};

  av::Codec l_audio_codec = av::findEncodingCodec(l_out_format, false);
  DOODLE_CHICK(!l_audio_codec.isNull(), "找不到 wav 音频编码器");

  av::AudioEncoderContext l_enc_ctx{l_audio_codec};
  l_enc_ctx.setSampleRate(k_audio_sample_rate);
  l_enc_ctx.setChannels(k_audio_channels);
  l_enc_ctx.setChannelLayout(AV_CH_LAYOUT_MONO);
  l_enc_ctx.setSampleFormat(AV_SAMPLE_FMT_S16);
  l_enc_ctx.setTimeBase(av::Rational{1, k_audio_sample_rate});
  {
    std::error_code l_ec{};
    l_enc_ctx.open(l_ec);
    DOODLE_CHICK(!l_ec, "音频编码器 {} 打开失败: {}", l_audio_codec.name(), l_ec.message());
  }

  av::FormatContext l_out_ctx{};
  l_out_ctx.setFormat(l_out_format);
  av::Stream l_out_audio_stream = l_out_ctx.addStream(l_enc_ctx);
  l_out_ctx.openOutput(in_wav.generic_string());
  l_out_ctx.writeHeader();

  std::uint32_t l_audio_bytes = 0;
  std::int64_t l_total_samples = 0;
  av::AudioResampler l_audio_resampler{};
  bool l_resampler_ready = false;

  const auto l_write_samples = [&](const av::AudioSamples& in_out_samples) {
    if (!in_out_samples || in_out_samples.samplesCount() <= 0) return;

    auto l_frame = in_out_samples;
    l_frame.setTimeBase(av::Rational{1, k_audio_sample_rate});
    l_frame.setPts(av::Timestamp{l_total_samples, av::Rational{1, k_audio_sample_rate}});

    if (auto l_pkt = l_enc_ctx.encode(l_frame)) {
      l_pkt.setStreamIndex(l_out_audio_stream.index());
      l_out_ctx.writePacket(l_pkt);
    }

    l_total_samples += in_out_samples.samplesCount();
    l_audio_bytes += static_cast<std::uint32_t>(in_out_samples.samplesCount()) * k_audio_channels * (k_audio_bits / 8);
  };

  const auto l_drain_audio = [&](const av::AudioSamples& in_samples) {
    if (!in_samples || in_samples.samplesCount() <= 0) return;

    if (!l_resampler_ready) {
      const auto l_src_layout = in_samples.channelsLayout();
      const auto l_src_rate   = in_samples.sampleRate();
      DOODLE_CHICK(
          l_src_layout != 0 && l_src_rate > 0, "音频源参数无效: layout={} rate={} channels={}", l_src_layout,
          l_src_rate, in_samples.channelsCount()
      );
      std::error_code l_ec{};
      const bool l_inited = l_audio_resampler.init(
          AV_CH_LAYOUT_MONO, k_audio_sample_rate, AV_SAMPLE_FMT_S16, l_src_layout, l_src_rate,
          in_samples.sampleFormat(), l_ec
      );
      DOODLE_CHICK(
          l_inited && l_audio_resampler.isValid(), "音频重采样器初始化失败: {} (layout={} rate={} fmt={})",
          l_ec.message(), l_src_layout, l_src_rate, static_cast<int>(in_samples.sampleFormat().get())
      );
      l_resampler_ready = true;
    }

    l_audio_resampler.push(in_samples);
    for (auto l_out = l_audio_resampler.pop(0); l_out; l_out = l_audio_resampler.pop(0)) l_write_samples(l_out);
  };

  while (auto l_pkt = l_format_ctx.readPacket()) {
    if (l_pkt.streamIndex() == l_video_stream.index()) {
      auto l_frame = l_dec_ctx.decode(l_pkt);
      if (!l_frame) continue;
      l_resampler.push(l_frame);
      l_write_pending();
    } else if (l_pkt.streamIndex() == l_audio_stream.index()) {
      l_drain_audio(l_audio_dec_ctx.decode(l_pkt));
    }
  }

  while (auto l_frame = l_dec_ctx.decode({})) {
    l_resampler.push(l_frame);
    l_write_pending();
  }
  while (auto l_samples = l_audio_dec_ctx.decode({})) l_drain_audio(l_samples);

  l_resampler.flush();
  l_write_pending();

  if (l_resampler_ready) {
    for (auto l_out = l_audio_resampler.pop(0); l_out; l_out = l_audio_resampler.pop(0)) l_write_samples(l_out);
  }

  const auto l_expected_samples = static_cast<std::int64_t>(
      std::llround(
          static_cast<double>(l_count) / static_cast<double>(in_target_fps) *
          static_cast<double>(k_audio_sample_rate)
      )
  );
  if (l_total_samples < l_expected_samples) {
    const auto l_pad = static_cast<int32_t>(l_expected_samples - l_total_samples);
    av::AudioSamples l_silence{AV_SAMPLE_FMT_S16, l_pad, AV_CH_LAYOUT_MONO, k_audio_sample_rate};
    std::memset(l_silence.data(0), 0, static_cast<std::size_t>(l_pad) * k_audio_channels * (k_audio_bits / 8));
    l_write_samples(l_silence);
  }

  {
    std::error_code l_ec{};
    auto l_pkt = l_enc_ctx.encode(l_ec);
    while (!l_ec && l_pkt) {
      l_pkt.setStreamIndex(l_out_audio_stream.index());
      l_out_ctx.writePacket(l_pkt);
      l_pkt = l_enc_ctx.encode(l_ec);
    }
    DOODLE_CHICK(!l_ec, "音频编码收尾失败: {}", l_ec.message());
  }
  l_out_ctx.writeTrailer();

  DOODLE_CHICK(l_audio_bytes > 0, "视频 {} 没有可用音频数据", in_video.generic_string());

  return l_count;
}

std::string read_log_code(const FSys::path& in_out_dir) {
  auto l_path = in_out_dir / std::string{k_log_file_name};
  if (!FSys::exists(l_path)) return {};

  try {
    auto const l_json = nlohmann::json::parse(FSys::ifstream{l_path});
    if (!l_json.is_object()) return {};

    const auto l_code   = l_json.contains("code") ? l_json.at("code").get<std::string>() : std::string{};
    const auto l_stage  = l_json.contains("stage") ? l_json.at("stage").get<std::string>() : std::string{};
    const auto l_detail = l_json.contains("detail") ? l_json.at("detail").get<std::string>() : std::string{};
    if (l_code.empty()) return {};
    return fmt::format("stage={} code={} detail={}", l_stage, l_code, l_detail);
  } catch (...) {
    return {};
  }
}

}  // namespace

void from_json(const nlohmann::json& in_json, face_capture_transcode_arg& out_obj) {
  in_json.at("preview_id").get_to(out_obj.preview_id_);
  in_json.at("ue_project").get_to(out_obj.ue_project_);
}

void to_json(nlohmann::json& in_json, const face_capture_transcode_arg& out_obj) {
  in_json["preview_id"]  = out_obj.preview_id_;
  in_json["ue_project"]  = out_obj.ue_project_;
}

boost::asio::awaitable<void> face_capture_transcode_arg::run() {
  const auto l_id_str = fmt::format("{}", preview_id_);
  DOODLE_CHICK(!preview_id_.is_nil(), "面部捕捉转码任务缺少 preview_id, 无法确定对应会话");

  const auto l_session_dir = session_dir_of(preview_id_);
  const auto l_video       = resolve_video(l_session_dir);
  const auto l_frames_dir  = l_session_dir / "frames";
  const auto l_curves_dir  = l_session_dir / "curves";

  DOODLE_CHICK(!l_video.empty(), "会话 {} 缺少录制视频 {}", l_id_str, l_session_dir.generic_string());

  if (FSys::exists(l_frames_dir)) FSys::remove_all(l_frames_dir);
  if (FSys::exists(l_curves_dir)) FSys::remove_all(l_curves_dir);
  FSys::create_directories(l_frames_dir);

  SPDLOG_LOGGER_INFO(logger_ptr_, "开始把 {} 转码为 png 序列", l_video.generic_string());
  const auto l_audio = l_session_dir / "audio.wav";
  if (FSys::exists(l_audio)) FSys::remove(l_audio);
  const auto l_frame_count =
      decode_to_png_and_wav(l_video, l_frames_dir, l_audio, face_capture::k_fps);
  SPDLOG_LOGGER_INFO(
      logger_ptr_, "转码完成 {} 帧, 输出目录 {}", l_frame_count, l_frames_dir.generic_string()
  );

  DOODLE_CHICK(l_frame_count >= 2, "会话 {} 只解码出 {} 帧, 无法解算动捕", l_id_str, l_frame_count);

  FSys::create_directories(l_curves_dir);

  DOODLE_CHICK(!ue_project_.empty(), "面部捕捉转码任务缺少 ue_project, 无法运行 UE 解算");

  const auto l_ue_arg = std::vector<std::string>{
      ue_project_.generic_string(),
      "-run=MetaHumanFace",
      "-AllowCommandletRendering",
      "-RenderOffscreen",
      fmt::format("-FramesDir={}", l_frames_dir.generic_string()),
      fmt::format("-OutDir={}", l_curves_dir.generic_string()),
      fmt::format("-FPS={}/{}", face_capture::k_fps, 1),
      fmt::format("-Audio={}", l_audio.generic_string()),
      fmt::format(R"(-abslog={})", (l_session_dir / "face_capture_ue.log").generic_string()),
      "-KeepTongueSolve",
      "-stdout",
      "-nosplash",
  };

  SPDLOG_LOGGER_INFO(logger_ptr_, "开始运行 UE 解算面部动捕 {}", l_ue_arg);
  try {
    co_await async_run_ue(l_ue_arg, logger_ptr_, kitsu_client_, true);
  } catch (const doodle_error& in_err) {
    const auto l_log_code = read_log_code(l_curves_dir);
    if (l_log_code.empty()) throw;
    throw_exception(doodle_error{"UE 面部动捕解算失败: {}; 原因: {}", in_err.what(), l_log_code});
  }

  const auto l_curves = l_curves_dir / std::string{k_curves_file_name};
  DOODLE_CHICK(
      FSys::exists(l_curves), "UE 解算完成但未生成动捕文件 {}, 日志: {}", l_curves.generic_string(),
      read_log_code(l_curves_dir)
  );

  SPDLOG_LOGGER_INFO(logger_ptr_, "面部动捕解算完成 {}", l_curves.generic_string());
  co_return;
}

}  // namespace doodle
