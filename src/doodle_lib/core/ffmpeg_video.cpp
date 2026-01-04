#include "ffmpeg_video.h"

#include "doodle_core/exception/exception.h"

#include <avcpp/audioresampler.h>
#include <avcpp/codec.h>
#include <avcpp/codeccontext.h>
#include <avcpp/filtergraph.h>
#include <avcpp/formatcontext.h>
#include <avcpp/frame.h>
#include <avcpp/stream.h>
#include <avcpp/timestamp.h>
#include <avcpp/videorescaler.h>
#include <cstdint>
#include <filesystem>
#include <libavcodec/codec_id.h>
#include <memory>
#include <opencv2/core/utility.hpp>
#include <opencv2/freetype.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <optional>
#include <rational.h>
#include <system_error>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
}

namespace doodle {

namespace {

auto read_next_packet_for_stream(av::FormatContext& ctx, int desired_stream_index) -> std::optional<av::Packet> {
  std::error_code ec{};

  while (true) {
    auto pkt = ctx.readPacket();
    if (pkt.isNull()) return std::nullopt;

    if (!pkt) continue;
    if (pkt.streamIndex() != desired_stream_index) continue;

    return pkt;
  }
}

auto pick_first_supported_pix_fmt(const av::Codec& codec, av::PixelFormat fallback) -> av::PixelFormat {
  auto fmts = codec.supportedPixelFormats();
  if (!fmts.empty() && fmts.front() != av::PixelFormat{AV_PIX_FMT_NONE}) {
    return fmts.front();
  }
  return fallback;
}

auto pick_first_supported_sample_fmt(const av::Codec& codec, av::SampleFormat fallback) -> av::SampleFormat {
  auto fmts = codec.supportedSampleFormats();
  if (!fmts.empty() && fmts.front() != av::SampleFormat{AV_SAMPLE_FMT_NONE}) {
    return fmts.front();
  }
  return fallback;
}

auto pick_channel_layout(uint64_t preferred, const av::Codec& codec) -> uint64_t {
  auto layouts = codec.supportedChannelLayouts();
  if (layouts.empty()) {
    return preferred != 0 ? preferred : static_cast<uint64_t>(AV_CH_LAYOUT_STEREO);
  }
  for (auto v : layouts) {
    if (v == preferred) {
      return preferred;
    }
  }
  return layouts.front();
}

std::uint64_t av_get_default_channel_layout(int nb_channels) {
  AVChannelLayout ch_layout{};
  av_channel_layout_default(&ch_layout, nb_channels);
  if (ch_layout.order == AV_CHANNEL_ORDER_NATIVE) {
    return ch_layout.u.mask;
  } else {
    switch (nb_channels) {
      case 1:
        return AV_CH_LAYOUT_MONO;
      case 2:
        return AV_CH_LAYOUT_STEREO;
      case 3:
        return AV_CH_LAYOUT_SURROUND;
      case 4:
        return AV_CH_LAYOUT_4POINT0;
      case 5:
        return AV_CH_LAYOUT_5POINT0;
      case 6:
        return AV_CH_LAYOUT_5POINT1;
      case 8:
        return AV_CH_LAYOUT_7POINT1;
      default:
        return AV_CH_LAYOUT_MONO;
    }
  }
}

}  // namespace
class ffmpeg_video::impl {
 public:
  impl()  = default;
  ~impl() = default;

 private:
  // 输出视频
  struct {
    av::FormatContext format_context_;

    av::Stream video_stream_;
    av::Codec h264_codec_;
    av::VideoEncoderContext video_enc_ctx_;

    std::int64_t video_frame_index{0};
    // 音频流
    av::Stream audio_stream_;
    av::Codec audio_codec_;
    av::AudioEncoderContext audio_enc_ctx_;
    std::int64_t audio_frame_index{0};
  } output_handle_;

  // 输入视频
  struct {
    av::FormatContext format_context_;
    av::Stream video_stream_;
    av::Codec video_codec_;
    av::VideoDecoderContext video_dec_ctx_;
  } input_video_handle_;

  struct subtitle_handle_t {
    av::FilterGraph graph_{};
    av::FilterContext buffersrc_ctx_{};
    av::BufferSrcFilterContext buffersrc_{};
    av::FilterContext subtitles_ctx_{};
    av::FilterContext buffersink_ctx_{};
    av::BufferSinkFilterContext buffersink_{};
    bool configured_{false};
  };

  std::unique_ptr<subtitle_handle_t> subtitle_handle_;

  // 音频组件
  struct {
    av::FormatContext format_context_;
    av::Stream stream_;
    av::Codec codec_;
    av::Stream out_stream_;
    av::AudioDecoderContext dec_ctx_;
    av::AudioResampler audio_resampler_;
  } audio_handle_;

  // 集数文件(这是一个mp4文件，里面有视频轨道, 无音频轨道)
  struct {
    av::FormatContext format_context_;
    av::VideoDecoderContext video_dec_ctx_;
    av::Stream stream_;
    av::Codec codec_;
  } episodes_name_handle_;
  // 片头 包含一个视频流和一个音频流
  struct {
    av::FormatContext format_context_;
    av::VideoDecoderContext video_dec_ctx_;
    av::Stream video_stream_;
    av::Codec video_codec_;
    av::Stream audio_stream_;
    av::Codec audio_codec_;
  } intro_handle_;
  // 片尾 包含一个视频流和一个音频流
  struct {
    av::FormatContext format_context_;
    av::VideoDecoderContext video_dec_ctx_;
    av::Stream video_stream_;
    av::Codec video_codec_;
    av::Stream audio_stream_;
    av::Codec audio_codec_;
  } outro_handle_;

  constexpr static int g_fps = 25;

  const av::Rational& get_video_time_base() const {
    static const av::Rational l_video_tb{1, g_fps};
    return l_video_tb;
  }

  void open_input_video(const FSys::path& in_path) {
    input_video_handle_.format_context_.openInput(in_path.string());
    input_video_handle_.format_context_.findStreamInfo();

    for (size_t i = 0; i < input_video_handle_.format_context_.streamsCount(); ++i) {
      auto st = input_video_handle_.format_context_.stream(i);
      if (st.isVideo()) {
        input_video_handle_.video_stream_ = st;
        break;
      }
    }
    DOODLE_CHICK(input_video_handle_.video_stream_.isValid(), "ffmpeg_video: input has no video stream");
    input_video_handle_.video_codec_ = input_video_handle_.video_stream_.codecParameters().decodingCodec();
    DOODLE_CHICK(!input_video_handle_.video_codec_.isNull(), "ffmpeg_video: cannot find video decoder");
    DOODLE_CHICK(input_video_handle_.video_codec_.isDecoder(), "ffmpeg_video: video decoder is not decoder");

    input_video_handle_.video_dec_ctx_ =
        av::VideoDecoderContext{input_video_handle_.video_stream_, input_video_handle_.video_codec_};
    input_video_handle_.video_dec_ctx_.open();
  }

  void open_output_video(const FSys::path& out_path) {
    output_handle_.format_context_.openOutput(out_path.string());

    constexpr static int k_fps = 25;
    const static av::Rational l_video_tb{1, k_fps};
    output_handle_.video_enc_ctx_ = av::VideoEncoderContext{output_handle_.h264_codec_};
    output_handle_.video_enc_ctx_.setWidth(input_video_handle_.video_dec_ctx_.width());
    output_handle_.video_enc_ctx_.setHeight(input_video_handle_.video_dec_ctx_.height());
    output_handle_.video_enc_ctx_.setTimeBase(l_video_tb);
    output_handle_.video_enc_ctx_.setPixelFormat(
        pick_first_supported_pix_fmt(output_handle_.h264_codec_, input_video_handle_.video_dec_ctx_.pixelFormat())
    );
    output_handle_.video_enc_ctx_.open();

    output_handle_.video_stream_ = output_handle_.format_context_.addStream(output_handle_.video_enc_ctx_);
    output_handle_.video_stream_.setTimeBase(l_video_tb);
    output_handle_.video_stream_.setFrameRate(av::Rational{k_fps, 1});
    output_handle_.video_stream_.setAverageFrameRate(av::Rational{k_fps, 1});
  }

  void open_output_audio() {
    DOODLE_CHICK(audio_handle_.stream_.isValid(), "ffmpeg_video: audio stream is not set");

    output_handle_.audio_enc_ctx_.setCodec(av::findEncodingCodec(AV_CODEC_ID_AAC));
    output_handle_.audio_enc_ctx_.setSampleRate(audio_handle_.dec_ctx_.sampleRate());
    output_handle_.audio_enc_ctx_.setChannelLayout(
        pick_channel_layout(audio_handle_.dec_ctx_.channelLayout(), audio_handle_.dec_ctx_.codec())
    );
    output_handle_.audio_enc_ctx_.setChannels(audio_handle_.dec_ctx_.channelLayout());
    output_handle_.audio_enc_ctx_.setSampleFormat(
        pick_first_supported_sample_fmt(audio_handle_.dec_ctx_.codec(), audio_handle_.dec_ctx_.sampleFormat())
    );
    output_handle_.audio_enc_ctx_.setTimeBase(av::Rational{1, audio_handle_.dec_ctx_.sampleRate()});
    output_handle_.audio_enc_ctx_.open();

    output_handle_.audio_stream_ = output_handle_.format_context_.addStream(output_handle_.audio_enc_ctx_);
    output_handle_.audio_stream_.setTimeBase(av::Rational{1, audio_handle_.dec_ctx_.sampleRate()});

    audio_handle_.audio_resampler_.init(
        output_handle_.audio_enc_ctx_.channelLayout(), output_handle_.audio_enc_ctx_.sampleRate(),
        output_handle_.audio_enc_ctx_.sampleFormat(), audio_handle_.dec_ctx_.channelLayout(),
        audio_handle_.dec_ctx_.sampleRate(), audio_handle_.dec_ctx_.sampleFormat()
    );
  }

 public:
  void open(const FSys::path& in_path, const FSys::path& out_path) {
    open_input_video(in_path);
    open_output_video(out_path);
  }

  // 添加音频轨道, 传入 MP4 文件路径, 提取音频轨道, 检查必须为 AAC 编码, 并将流直接复制到输出文件
  void add_audio(const FSys::path& in_audio_path) {
    audio_handle_.format_context_.openInput(in_audio_path.string());
    audio_handle_.format_context_.findStreamInfo();

    // 获取音频长度并进行检查
    auto l_video_duration = input_video_handle_.format_context_.duration();
    auto l_audio_duration = audio_handle_.format_context_.duration();
    DOODLE_CHICK(
        l_audio_duration == l_video_duration,
        std::format(
            "ffmpeg_video: audio duration {} does not match video duration {}", l_audio_duration, l_video_duration
        )
    );

    for (size_t i = 0; i < audio_handle_.format_context_.streamsCount(); ++i) {
      auto st = audio_handle_.format_context_.stream(i);
      if (st.isAudio()) {
        audio_handle_.stream_ = st;
        break;
      }
    }
    DOODLE_CHICK(audio_handle_.stream_.isValid(), "ffmpeg_video: audio input has no audio stream");

    audio_handle_.out_stream_ = output_handle_.format_context_.addStream();
    audio_handle_.out_stream_.setTimeBase(audio_handle_.stream_.timeBase());
    audio_handle_.out_stream_.setFrameRate(audio_handle_.stream_.frameRate());
    audio_handle_.out_stream_.codecParameters().copyFrom(audio_handle_.stream_.codecParameters());

    audio_handle_.codec_ = audio_handle_.stream_.codecParameters().decodingCodec();
    DOODLE_CHICK(!audio_handle_.codec_.isNull(), "ffmpeg_video: cannot find audio decoder");
    audio_handle_.dec_ctx_ = av::AudioDecoderContext{audio_handle_.stream_, audio_handle_.codec_};
    audio_handle_.dec_ctx_.open();

    open_output_audio();
  }

  void add_subtitle(const FSys::path& in_subtitle_path) {
    DOODLE_CHICK(!in_subtitle_path.empty(), "字幕路径为空");
    DOODLE_CHICK(FSys::exists(in_subtitle_path), std::format("字幕文件不存在: {}", in_subtitle_path.string()));
    DOODLE_CHICK(
        FSys::is_regular_file(in_subtitle_path), std::format("字幕路径不是文件: {}", in_subtitle_path.string())
    );

    auto ext = in_subtitle_path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
    DOODLE_CHICK(ext == ".srt", "文件格式不支持, 仅支持 .srt 字幕文件");

    // 使用 avfilter 的 subtitles 滤镜渲染 .srt (依赖 FFmpeg 编译启用 libass)
    // 滤镜图: buffer -> subtitles -> buffersink

    const auto w = output_handle_.video_enc_ctx_.width();
    const auto h = output_handle_.video_enc_ctx_.height();
    DOODLE_CHICK(w > 0 && h > 0, "ffmpeg_video: invalid video size for subtitle graph");

    const int pix_fmt = static_cast<int>(output_handle_.video_enc_ctx_.pixelFormat());
    const auto tb     = get_video_time_base();
    auto sar          = input_video_handle_.video_stream_.sampleAspectRatio();
    if (sar == av::Rational{}) sar = av::Rational{1, 1};

    const std::string buffer_args = std::format(
        "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}", w, h, pix_fmt, tb.getNumerator(),
        tb.getDenominator(), sar.getNumerator(), sar.getDenominator()
    );

    subtitle_handle_ = std::make_unique<subtitle_handle_t>();
    subtitle_handle_->graph_.setAutoConvert(0);

    const av::Filter buffer_filter{"buffer"};
    const av::Filter buffersink_filter{"buffersink"};
    const av::Filter subtitles_filter{"subtitles"};
    DOODLE_CHICK(buffer_filter, "ffmpeg_video: cannot find filter 'buffer'");
    DOODLE_CHICK(buffersink_filter, "ffmpeg_video: cannot find filter 'buffersink'");
    DOODLE_CHICK(subtitles_filter, "ffmpeg_video: cannot find filter 'subtitles' (FFmpeg needs libass)");

    subtitle_handle_->buffersrc_ctx_  = subtitle_handle_->graph_.createFilter(buffer_filter, "in", buffer_args);
    subtitle_handle_->buffersink_ctx_ = subtitle_handle_->graph_.createFilter(buffersink_filter, "out", "");

    // subtitles 的 filename 在 Windows 上包含 ':'，直接走 args 字符串容易被 ':' 分隔符影响。
    // 这里用 av_opt_set 设置 option，再 init。
    subtitle_handle_->subtitles_ctx_  = subtitle_handle_->graph_.allocFilter(subtitles_filter, "sub");
    const std::string subtitle_file   = in_subtitle_path.generic_string();
    {
      const int ret =
          av_opt_set(subtitle_handle_->subtitles_ctx_.raw(), "filename", subtitle_file.c_str(), AV_OPT_SEARCH_CHILDREN);
      DOODLE_CHICK(ret >= 0, std::format("ffmpeg_video: set subtitles filename failed: {}", subtitle_file));
    }
    subtitle_handle_->subtitles_ctx_.init("");

    subtitle_handle_->buffersrc_ctx_.link(0, subtitle_handle_->subtitles_ctx_, 0);
    subtitle_handle_->subtitles_ctx_.link(0, subtitle_handle_->buffersink_ctx_, 0);

    subtitle_handle_->graph_.config();

    subtitle_handle_->buffersrc_  = av::BufferSrcFilterContext{subtitle_handle_->buffersrc_ctx_};
    subtitle_handle_->buffersink_ = av::BufferSinkFilterContext{subtitle_handle_->buffersink_ctx_};
    subtitle_handle_->configured_ = true;
  }

  void add_episodes_name(const FSys::path& in_episodes_name_path) {
    DOODLE_CHICK(!in_episodes_name_path.empty(), "集数名称路径为空");
    DOODLE_CHICK(
        FSys::exists(in_episodes_name_path), std::format("集数名称文件不存在: {}", in_episodes_name_path.string())
    );
    DOODLE_CHICK(
        FSys::is_regular_file(in_episodes_name_path),
        std::format("集数名称路径不是文件: {}", in_episodes_name_path.string())
    );

    episodes_name_handle_.format_context_.openInput(in_episodes_name_path.string());
    episodes_name_handle_.format_context_.findStreamInfo();

    for (size_t i = 0; i < episodes_name_handle_.format_context_.streamsCount(); ++i) {
      auto st = episodes_name_handle_.format_context_.stream(i);
      if (st.isVideo()) {
        episodes_name_handle_.stream_ = st;
        break;
      }
    }
    DOODLE_CHICK(episodes_name_handle_.stream_.isValid(), "ffmpeg_video: episodes name input has no video stream");

    episodes_name_handle_.codec_ = episodes_name_handle_.stream_.codecParameters().decodingCodec();
    DOODLE_CHICK(!episodes_name_handle_.codec_.isNull(), "ffmpeg_video: cannot find video decoder for episodes name");
    DOODLE_CHICK(
        episodes_name_handle_.codec_.isDecoder(), "ffmpeg_video: video decoder for episodes name is not decoder"
    );
    episodes_name_handle_.video_dec_ctx_ =
        av::VideoDecoderContext{episodes_name_handle_.stream_, episodes_name_handle_.codec_};
    episodes_name_handle_.video_dec_ctx_.open();
  }
  void add_intro(const FSys::path& in_intro_path) {
    DOODLE_CHICK(!in_intro_path.empty(), "片头路径为空");
    DOODLE_CHICK(FSys::exists(in_intro_path), std::format("片头文件不存在: {}", in_intro_path.string()));
    DOODLE_CHICK(FSys::is_regular_file(in_intro_path), std::format("片头路径不是文件: {}", in_intro_path.string()));

    intro_handle_.format_context_.openInput(in_intro_path.string());
    intro_handle_.format_context_.findStreamInfo();

    for (size_t i = 0; i < intro_handle_.format_context_.streamsCount(); ++i) {
      auto st = intro_handle_.format_context_.stream(i);
      if (st.isVideo()) {
        intro_handle_.video_stream_ = st;
      } else if (st.isAudio()) {
        intro_handle_.audio_stream_ = st;
      }
    }
    DOODLE_CHICK(intro_handle_.video_stream_.isValid(), "ffmpeg_video: intro input has no video stream");
    DOODLE_CHICK(intro_handle_.audio_stream_.isValid(), "ffmpeg_video: intro input has no audio stream");
    intro_handle_.video_codec_ = intro_handle_.video_stream_.codecParameters().decodingCodec();
    DOODLE_CHICK(!intro_handle_.video_codec_.isNull(), "ffmpeg_video: cannot find video decoder for intro");
    DOODLE_CHICK(intro_handle_.video_codec_.isDecoder(), "ffmpeg_video: video decoder for intro is not decoder");

    intro_handle_.video_dec_ctx_ = av::VideoDecoderContext{intro_handle_.video_stream_, intro_handle_.video_codec_};
    intro_handle_.video_dec_ctx_.open();
  }
  void add_outro(const FSys::path& in_outro_path) {
    DOODLE_CHICK(!in_outro_path.empty(), "片尾路径为空");
    DOODLE_CHICK(FSys::exists(in_outro_path), std::format("片尾文件不存在: {}", in_outro_path.string()));
    DOODLE_CHICK(FSys::is_regular_file(in_outro_path), std::format("片尾路径不是文件: {}", in_outro_path.string()));

    outro_handle_.format_context_.openInput(in_outro_path.string());
    outro_handle_.format_context_.findStreamInfo();

    for (size_t i = 0; i < outro_handle_.format_context_.streamsCount(); ++i) {
      auto st = outro_handle_.format_context_.stream(i);
      if (st.isVideo()) {
        outro_handle_.video_stream_ = st;
      } else if (st.isAudio()) {
        outro_handle_.audio_stream_ = st;
      }
    }
    DOODLE_CHICK(outro_handle_.video_stream_.isValid(), "ffmpeg_video: outro input has no video stream");
    DOODLE_CHICK(outro_handle_.audio_stream_.isValid(), "ffmpeg_video: outro input has no audio stream");

    outro_handle_.video_codec_ = outro_handle_.video_stream_.codecParameters().decodingCodec();
    DOODLE_CHICK(!outro_handle_.video_codec_.isNull(), "ffmpeg_video: cannot find video decoder for outro");
    DOODLE_CHICK(outro_handle_.video_codec_.isDecoder(), "ffmpeg_video: video decoder for outro is not decoder");

    outro_handle_.video_dec_ctx_ = av::VideoDecoderContext{outro_handle_.video_stream_, outro_handle_.video_codec_};
    outro_handle_.video_dec_ctx_.open();
  }

  void encode_video_frame(const av::VideoFrame& in_frame) {
    auto out_pkt = output_handle_.video_enc_ctx_.encode(in_frame);
    if (out_pkt && !out_pkt.isNull()) {
      out_pkt.setTimeBase(get_video_time_base());
      out_pkt.setStreamIndex(output_handle_.video_stream_.index());
      output_handle_.format_context_.writePacket(out_pkt);
    }
  }
  void encode_audio_frame(av::AudioSamples& in_frame) {
    in_frame.setTimeBase(av::Rational{1, output_handle_.audio_enc_ctx_.sampleRate()});
    in_frame.setPts(av::Timestamp{output_handle_.audio_frame_index, in_frame.timeBase()});
    output_handle_.audio_frame_index += in_frame.samplesCount();
    auto out_pkt = output_handle_.audio_enc_ctx_.encode(in_frame);
    if (out_pkt && !out_pkt.isNull()) {
      out_pkt.setTimeBase(output_handle_.audio_stream_.timeBase());
      out_pkt.setStreamIndex(output_handle_.audio_stream_.index());
      output_handle_.format_context_.writePacket(out_pkt);
    }
  }
  void flush_video_encoder() {
    const int l_out_video_index = output_handle_.video_stream_.index();
    // Flush video encoder
    while (true) {
      auto out_pkt = output_handle_.video_enc_ctx_.encode();
      if (!out_pkt || out_pkt.isNull()) {
        break;
      }
      out_pkt.setTimeBase(get_video_time_base());
      out_pkt.setStreamIndex(l_out_video_index);
      output_handle_.format_context_.writePacket(out_pkt);
    }
  }

  void process_episodes() {
    // -----------------
    // Encode video packets
    // -----------------
    while (auto pkt_opt = read_next_packet_for_stream(
               episodes_name_handle_.format_context_, episodes_name_handle_.stream_.index()
           )) {
      auto frame = episodes_name_handle_.video_dec_ctx_.decode(*pkt_opt);
      if (!frame) {
        continue;
      }
      frame.setTimeBase(get_video_time_base());
      frame.setPts(av::Timestamp{output_handle_.video_frame_index++, get_video_time_base()});
      encode_video_frame(frame);
    }
  }
  void process_intro() {
    // -----------------
    // Encode video packets
    // -----------------
    while (auto pkt_opt =
               read_next_packet_for_stream(intro_handle_.format_context_, intro_handle_.video_stream_.index())) {
      auto frame = intro_handle_.video_dec_ctx_.decode(*pkt_opt);
      if (!frame) {
        continue;
      }
      frame.setTimeBase(get_video_time_base());
      frame.setPts(av::Timestamp{output_handle_.video_frame_index++, get_video_time_base()});
      encode_video_frame(frame);
    }
  }
  void process_outro() {
    // -----------------
    // Encode video packets
    // -----------------
    while (auto pkt_opt =
               read_next_packet_for_stream(outro_handle_.format_context_, outro_handle_.video_stream_.index())) {
      auto frame = outro_handle_.video_dec_ctx_.decode(*pkt_opt);
      if (!frame) {
        continue;
      }
      frame.setTimeBase(get_video_time_base());
      frame.setPts(av::Timestamp{output_handle_.video_frame_index++, get_video_time_base()});
      encode_video_frame(frame);
    }
  }

  void process_out_video() {
    const int l_out_video_index = output_handle_.video_stream_.index();

    // -----------------
    // Encode video packets
    // -----------------
    while (auto pkt_opt = read_next_packet_for_stream(
               input_video_handle_.format_context_, input_video_handle_.video_stream_.index()
           )) {
      auto frame = input_video_handle_.video_dec_ctx_.decode(*pkt_opt);
      if (!frame) {
        continue;
      }
      frame.setTimeBase(get_video_time_base());
      frame.setPts(av::Timestamp{output_handle_.video_frame_index++, get_video_time_base()});

      if (subtitle_handle_ && subtitle_handle_->configured_) {
        subtitle_handle_->buffersrc_.writeVideoFrame(frame);
        while (true) {
          av::VideoFrame filtered;
          if (!subtitle_handle_->buffersink_.getVideoFrame(filtered)) break;
          filtered.setTimeBase(get_video_time_base());
          encode_video_frame(filtered);
        }
      } else {
        encode_video_frame(frame);
      }
    }

    // Flush subtitle filter
    if (subtitle_handle_ && subtitle_handle_->configured_) {
      (void)av_buffersrc_add_frame_flags(subtitle_handle_->buffersrc_ctx_.raw(), nullptr, 0);
      while (true) {
        av::VideoFrame filtered;
        if (!subtitle_handle_->buffersink_.getVideoFrame(filtered)) break;
        filtered.setTimeBase(get_video_time_base());
        auto out_pkt = output_handle_.video_enc_ctx_.encode(filtered);
        if (out_pkt && !out_pkt.isNull()) {
          out_pkt.setTimeBase(get_video_time_base());
          out_pkt.setStreamIndex(l_out_video_index);
          output_handle_.format_context_.writePacket(out_pkt);
        }
      }
    }
  }

  void process_audio() {
    const auto l_autio_frame_size = audio_handle_.dec_ctx_.frameSize();
    while (auto pkt_opt = read_next_packet_for_stream(audio_handle_.format_context_, audio_handle_.stream_.index())) {
      auto frame = audio_handle_.dec_ctx_.decode(*pkt_opt);
      if (!frame) {
        continue;
      }
      audio_handle_.audio_resampler_.push(frame);
      while (true) {
        auto l_out = audio_handle_.audio_resampler_.pop(l_autio_frame_size);
        if (!l_out) break;
        encode_audio_frame(l_out);
      }
    }
    auto l_out = audio_handle_.audio_resampler_.pop(0);
    if (l_out) encode_audio_frame(l_out);
  }

  void process() {
    output_handle_.format_context_.writeHeader();
    if (intro_handle_.video_stream_.isValid()) {
      process_intro();
    }
    if (episodes_name_handle_.stream_.isValid()) {
      process_episodes();
    }
    // 处理主视频流
    process_out_video();
    if (outro_handle_.video_stream_.isValid()) {
      process_outro();
    }

    if (!audio_handle_.format_context_.isNull()) {
      process_audio();
    }

    flush_video_encoder();

    output_handle_.format_context_.writeTrailer();
  }
};

ffmpeg_video::ffmpeg_video(const FSys::path& in_video_path, const FSys::path& in_out_path)
    : impl_(std::make_unique<impl>()), video_path_(in_video_path), out_path_(in_out_path) {}

ffmpeg_video::ffmpeg_video() : impl_(std::make_unique<impl>()) {}

ffmpeg_video::~ffmpeg_video() = default;

void ffmpeg_video::process() {
  out_path_.replace_extension(".mp4");
  DOODLE_CHICK(!video_path_.empty() && FSys::exists(video_path_), "ffmpeg_video: video path is empty or not exists");

  impl_->open(video_path_, out_path_);
  if (!subtitle_path_.empty() && FSys::exists(subtitle_path_)) {
    impl_->add_subtitle(subtitle_path_);
  }
  if (!audio_path_.empty() && FSys::exists(audio_path_)) {
    impl_->add_audio(audio_path_);
  }
  if (!episodes_name_path_.empty() && FSys::exists(episodes_name_path_)) {
    impl_->add_episodes_name(episodes_name_path_);
  }
  if (!intro_path_.empty() && FSys::exists(intro_path_) && !outro_path_.empty() && FSys::exists(outro_path_)) {
    impl_->add_intro(intro_path_);
    impl_->add_outro(outro_path_);
  }

  impl_->process();
}

void ffmpeg_video::preprocess_wav_to_aac(const FSys::path& in_wav_path, const FSys::path& in_out_path) {
  DOODLE_CHICK(!in_wav_path.empty() && FSys::exists(in_wav_path), "ffmpeg_video: audio path is empty or not exists");
  DOODLE_CHICK(!in_out_path.empty(), "ffmpeg_video: output path is empty");
  auto out_path = in_out_path;
  ;
  out_path.replace_extension(".mp4");

  av::FormatContext l_input_ctx{};
  l_input_ctx.openInput(in_wav_path.string());
  l_input_ctx.findStreamInfo();

  av::FormatContext l_output_ctx{};
  l_output_ctx.openOutput(out_path.string());

  av::Stream l_in_audio_stream{};
  for (size_t i = 0; i < l_input_ctx.streamsCount(); ++i) {
    auto st = l_input_ctx.stream(i);
    if (st.isAudio()) {
      l_in_audio_stream = st;
      break;
    }
  }
  DOODLE_CHICK(l_in_audio_stream.isValid(), "ffmpeg_video: audio input has no audio stream");

  av::Codec l_in_codec = l_in_audio_stream.codecParameters().decodingCodec();
  DOODLE_CHICK(!l_in_codec.isNull(), "ffmpeg_video: cannot find audio decoder");
  DOODLE_CHICK(l_in_codec.canDecode(), "ffmpeg_video: cannot find audio decoder");

  av::AudioDecoderContext l_dec_ctx{l_in_audio_stream, l_in_codec};
  l_dec_ctx.open();

  av::AudioEncoderContext l_enc_ctx{};
  l_enc_ctx.setCodec(av::findEncodingCodec(AV_CODEC_ID_AAC));

  const int l_dst_sample_rate = l_dec_ctx.sampleRate() > 0 ? l_dec_ctx.sampleRate() : 48000;
  const std::uint64_t l_src_channel_layout =
      l_dec_ctx.channelLayout() != 0 ? l_dec_ctx.channelLayout() : av_get_default_channel_layout(l_dec_ctx.channels());
  const uint64_t l_dst_layout = pick_channel_layout(l_src_channel_layout, l_enc_ctx.codec());
  const av::SampleFormat l_dst_sample_fmt =
      pick_first_supported_sample_fmt(l_enc_ctx.codec(), av::SampleFormat{AV_SAMPLE_FMT_FLTP});

  l_enc_ctx.setSampleRate(l_dst_sample_rate);
  l_enc_ctx.setChannelLayout(l_dst_layout);
  l_enc_ctx.setSampleFormat(l_dst_sample_fmt);
  const av::Rational l_audio_tb{1, l_enc_ctx.sampleRate()};
  l_enc_ctx.setTimeBase(l_audio_tb);
  l_enc_ctx.open();

  av::Stream l_out_stream = l_output_ctx.addStream(l_enc_ctx);
  l_out_stream.setTimeBase(l_audio_tb);

  av::AudioResampler l_resampler{};
  if (l_dec_ctx.sampleRate() != l_enc_ctx.sampleRate() || l_dec_ctx.channelLayout() != l_enc_ctx.channelLayout() ||
      l_dec_ctx.sampleFormat() != l_enc_ctx.sampleFormat()) {
    l_resampler.init(
        l_enc_ctx.channelLayout(), l_enc_ctx.sampleRate(), l_enc_ctx.sampleFormat(), l_src_channel_layout,
        l_dec_ctx.sampleRate(), l_dec_ctx.sampleFormat()
    );
  }

  l_output_ctx.writeHeader();

  const int l_audio_frame_size    = l_enc_ctx.frameSize() > 0 ? l_enc_ctx.frameSize() : 1024;
  int64_t l_audio_samples_written = 0;

  auto encode_audio_samples       = [&](av::AudioSamples& samples) {
    samples.setTimeBase(l_audio_tb);
    samples.setPts(av::Timestamp{l_audio_samples_written, l_audio_tb});
    l_audio_samples_written += samples.samplesCount();
    auto out_pkt = l_enc_ctx.encode(samples);
    if (out_pkt && !out_pkt.isNull()) {
      out_pkt.setTimeBase(l_audio_tb);
      out_pkt.setStreamIndex(l_out_stream.index());
      l_output_ctx.writePacket(out_pkt);
    }
  };

  while (auto pkt_opt = read_next_packet_for_stream(l_input_ctx, l_in_audio_stream.index())) {
    auto samples = l_dec_ctx.decode(*pkt_opt);
    if (!samples) {
      continue;
    }

    // avcpp may report channelsLayout() as 0 with FFmpeg new channel layout API
    // (e.g. when ch_layout.order is not AV_CHANNEL_ORDER_NATIVE). AudioResampler
    // requires a stable, non-zero layout mask, so we synthesize a default one.
    if (samples.channelsLayout() == 0) {
      int channels = samples.channelsCount();
      if (channels <= 0) {
        channels = l_dec_ctx.channels();
      }
      if (channels <= 0) {
        channels = 2;
      }
      av::frame::set_channel_layout(samples.raw(), av_get_default_channel_layout(channels));
    }
    if (samples.sampleRate() <= 0) {
      av::frame::set_sample_rate(samples.raw(), l_dec_ctx.sampleRate());
    }

    if (l_resampler.isValid()) {
      l_resampler.push(samples);
      while (true) {
        auto out = l_resampler.pop(static_cast<size_t>(l_audio_frame_size));
        if (!out) {
          break;
        }
        encode_audio_samples(out);
      }
    } else {
      encode_audio_samples(samples);
    }
  }

  // Flush resampler delayed samples
  if (l_resampler.isValid()) {
    // 为 0 时会一次提取所有样本, 不需要循环
    auto out = l_resampler.pop(0);
    if (out) encode_audio_samples(out);
  }

  // Flush audio encoder
  while (true) {
    auto out_pkt = l_enc_ctx.encode();
    if (!out_pkt || out_pkt.isNull()) {
      break;
    }
    out_pkt.setTimeBase(l_audio_tb);
    out_pkt.setStreamIndex(l_out_stream.index());
    l_output_ctx.writePacket(out_pkt);
  }

  l_output_ctx.writeTrailer();
}

void ffmpeg_video::check_video_valid(const FSys::path& in_video_path) {
  DOODLE_CHICK(!in_video_path.empty(), "ffmpeg_video: video path is empty");
  DOODLE_CHICK(
      FSys::exists(in_video_path), std::format("ffmpeg_video: video file not exists: {}", in_video_path.string())
  );
  DOODLE_CHICK(
      FSys::is_regular_file(in_video_path),
      std::format("ffmpeg_video: video path is not a file: {}", in_video_path.string())
  );

  av::FormatContext l_input_ctx{};
  l_input_ctx.openInput(in_video_path.string());
  l_input_ctx.findStreamInfo();

  av::Stream l_in_video_stream{};
  av::Stream l_in_audio_stream{};
  for (size_t i = 0; i < l_input_ctx.streamsCount(); ++i) {
    auto st = l_input_ctx.stream(i);
    if (st.isVideo() && !l_in_video_stream.isValid()) {
      l_in_video_stream = st;
      continue;
    }
    if (st.isAudio() && !l_in_audio_stream.isValid()) {
      l_in_audio_stream = st;
      continue;
    }
  }
  DOODLE_CHICK(l_in_video_stream.isValid(), "ffmpeg_video: input has no video stream");

  // video decoder must exist
  av::Codec l_in_video_codec = l_in_video_stream.codecParameters().decodingCodec();
  DOODLE_CHICK(!l_in_video_codec.isNull(), "ffmpeg_video: cannot find video decoder");
  DOODLE_CHICK(l_in_video_codec.canDecode(), "ffmpeg_video: video decoder cannot decode");

  // audio stream is optional, but if present it must be AAC + stereo (2 channels)
  if (l_in_audio_stream.isValid()) {
    av::Codec l_in_audio_codec = l_in_audio_stream.codecParameters().decodingCodec();
    DOODLE_CHICK(!l_in_audio_codec.isNull(), "ffmpeg_video: cannot find audio decoder");
    DOODLE_CHICK(l_in_audio_codec.canDecode(), "ffmpeg_video: audio decoder cannot decode");

    const AVCodec* raw_codec = l_in_audio_codec.raw();
    DOODLE_CHICK(raw_codec != nullptr, "ffmpeg_video: audio decoder raw codec is null");
    DOODLE_CHICK(raw_codec->id == AV_CODEC_ID_AAC, "ffmpeg_video: audio codec is not AAC");

    av::AudioDecoderContext l_dec_ctx{l_in_audio_stream, l_in_audio_codec};
    l_dec_ctx.open();
    DOODLE_CHICK(l_dec_ctx.channels() == 2, "ffmpeg_video: audio channel is not stereo");
  }
}

}  // namespace doodle