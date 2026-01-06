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
#include <channellayout.h>
#include <cstdint>
#include <filesystem>
#include <filtercontext.h>
#include <fmt/format.h>
#include <libavcodec/codec_id.h>
#include <memory>
#include <opencv2/core/utility.hpp>
#include <opencv2/freetype.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <optional>
#include <rational.h>
#include <string>
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
  for (auto&& fmt : fmts) {
    if (fmt != av::PixelFormat{AV_PIX_FMT_NONE}) return fmt;
  }
  return fallback;
}

auto pick_first_supported_sample_fmt(const av::Codec& codec, av::SampleFormat fallback) -> av::SampleFormat {
  auto fmts = codec.supportedSampleFormats();
  for (auto&& fmt : fmts) {
    if (fmt != av::SampleFormat{AV_SAMPLE_FMT_NONE}) return fmt;
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
  struct base_t {
    av::FormatContext format_context_;

    av::VideoDecoderContext video_dec_ctx_;
    av::Stream video_stream_;
    av::Codec video_codec_;

    av::AudioDecoderContext audio_dec_ctx_;
    av::AudioResampler audio_resampler_;
    av::Stream audio_stream_;
    av::Codec audio_codec_;
    // 这里储存一下音频的 channel layout, 因为有些音频流没有 channel layout 信息, 需要我们手动指定
    av::ChannelLayout audio_channel_layout_{0};

    void open_format_context(const FSys::path& in_path) {
      format_context_.openInput(in_path.string());
      format_context_.findStreamInfo();
    }

    void open_video_context() {
      for (size_t i = 0; i < format_context_.streamsCount(); ++i) {
        auto st = format_context_.stream(i);
        if (st.isVideo()) {
          video_stream_ = st;
          break;
        }
      }
      DOODLE_CHICK(video_stream_.isValid(), "ffmpeg_video: input has no video stream");
      video_codec_ = video_stream_.codecParameters().decodingCodec();
      DOODLE_CHICK(!video_codec_.isNull(), "ffmpeg_video: cannot find video decoder");
      DOODLE_CHICK(video_codec_.isDecoder(), "ffmpeg_video: video decoder is not decoder");

      video_dec_ctx_ = av::VideoDecoderContext{video_stream_, video_codec_};
      video_dec_ctx_.open();
    }

    void open_audio_context() {
      for (size_t i = 0; i < format_context_.streamsCount(); ++i) {
        auto st = format_context_.stream(i);
        if (st.isAudio()) {
          audio_stream_ = st;
          break;
        }
      }
      DOODLE_CHICK(audio_stream_.isValid(), "ffmpeg_video: input has no audio stream");
      audio_codec_ = audio_stream_.codecParameters().decodingCodec();
      DOODLE_CHICK(!audio_codec_.isNull(), "ffmpeg_video: cannot find audio decoder");
      DOODLE_CHICK(audio_codec_.isDecoder(), "ffmpeg_video: audio decoder is not decoder");

      audio_dec_ctx_ = av::AudioDecoderContext{audio_stream_, audio_codec_};
      audio_dec_ctx_.open();
      audio_channel_layout_ = audio_dec_ctx_.channelLayout2().layout() == 0
                                  ? av::ChannelLayout{audio_dec_ctx_.channelLayout2().channels()}
                                  : av::ChannelLayout{audio_dec_ctx_.channelLayout2()};
    }

    void add_audio_resampler(const av::AudioEncoderContext& out_codec) {
      if (out_codec.channelLayout2() != audio_channel_layout_ ||
          out_codec.sampleRate() != audio_dec_ctx_.sampleRate() ||
          out_codec.sampleFormat() != audio_dec_ctx_.sampleFormat())
        audio_resampler_.init(
            out_codec.channelLayout2().layout(), out_codec.sampleRate(), out_codec.sampleFormat(),
            audio_channel_layout_.layout(), audio_dec_ctx_.sampleRate(), audio_dec_ctx_.sampleFormat()
        );
    }

    void process_output_video(ffmpeg_video::impl& parent) {
      while (auto pkt_opt = read_next_packet_for_stream(format_context_, video_stream_.index())) {
        auto frame = video_dec_ctx_.decode(*pkt_opt);
        if (!frame) continue;

        if (parent.subtitle_handle_ && parent.subtitle_handle_->configured_) {
          parent.subtitle_handle_->buffersrc_.writeVideoFrame(frame);
          av::VideoFrame filtered_frame{};
          while (parent.subtitle_handle_->buffersink_.getVideoFrame(filtered_frame))
            parent.encode_video_frame(filtered_frame);
        } else
          parent.encode_video_frame(frame);
      }
    }

    void process_output_audio(ffmpeg_video::impl& parent) {
      bool channel_layout_fixed = audio_dec_ctx_.channelLayout2() == audio_channel_layout_;

      while (auto pkt_opt = read_next_packet_for_stream(format_context_, audio_stream_.index())) {
        auto frame = audio_dec_ctx_.decode(*pkt_opt);
        if (!frame) continue;

        if (!channel_layout_fixed) {
          av::frame::set_channel_layout(frame.raw(), audio_channel_layout_.layout());
        }
        if (frame.sampleRate() <= 0) av::frame::set_sample_rate(frame.raw(), audio_dec_ctx_.sampleRate());

        if (audio_resampler_.isValid()) {
          audio_resampler_.push(frame);
          while (auto resampled_frame = audio_resampler_.pop(parent.output_handle_.audio_enc_ctx_.frameSize()))
            parent.encode_audio_frame(resampled_frame);
        } else
          parent.encode_audio_frame(frame);
      }

      if (audio_resampler_.isValid()) {
        auto l_pkt = audio_resampler_.pop(0);
        if (l_pkt) parent.encode_audio_frame(l_pkt);
      }
    }
  };
  // 输出视频
  struct out_video : base_t {
    av::Codec h264_codec_;
    av::VideoEncoderContext video_enc_ctx_;

    av::Timestamp video_next_pts_{};
    // 音频流
    av::AudioEncoderContext audio_enc_ctx_;

    av::Timestamp audio_next_pts_{};
  };
  out_video output_handle_;

  // 输入视频
  base_t input_video_handle_;

  struct subtitle_handle_t {
    av::FilterGraph graph_{};
    av::FilterContext buffersrc_ctx_{};
    av::BufferSrcFilterContext buffersrc_{};
    // 渲染字幕过滤器
    av::FilterContext subtitles_ctx_{};
    // 时间码过滤器
    av::FilterContext timecode_ctx_{};
    // 水印过滤器
    av::FilterContext watermark_ctx_{};

    av::FilterContext buffersink_ctx_{};
    av::BufferSinkFilterContext buffersink_{};
    bool configured_{false};
  };

  std::unique_ptr<subtitle_handle_t> subtitle_handle_;

  // 音频组件
  base_t audio_handle_;

  // 集数文件(这是一个mp4文件，里面有视频轨道, 无音频轨道)
  base_t episodes_name_handle_;
  // 片头 包含一个视频流和一个音频流
  base_t intro_handle_;
  // 片尾 包含一个视频流和一个音频流
  base_t outro_handle_;

  constexpr static int g_fps = 25;

  const av::Rational& get_video_time_base() const {
    static const av::Rational l_video_tb{1, g_fps};
    return l_video_tb;
  }

  void open_input_video(const FSys::path& in_path) {
    input_video_handle_.open_format_context(in_path);
    input_video_handle_.open_video_context();
  }

  void open_output_video(const FSys::path& out_path) {
    output_handle_.format_context_.openOutput(out_path.string());
    output_handle_.h264_codec_ = av::findEncodingCodec(AV_CODEC_ID_H264);
    DOODLE_CHICK(output_handle_.h264_codec_.isEncoder(), "ffmpeg_video: cannot find h264 encoder");

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
    output_handle_.video_next_pts_ = av::Timestamp{0, l_video_tb};
  }

  void open_output_audio() {
    output_handle_.audio_codec_ = av::findEncodingCodec(AV_CODEC_ID_AAC);
    DOODLE_CHICK(output_handle_.audio_codec_.isEncoder(), "ffmpeg_video: cannot find aac encoder");
    output_handle_.audio_enc_ctx_ = av::AudioEncoderContext{output_handle_.audio_codec_};
    DOODLE_CHICK(output_handle_.audio_enc_ctx_.isValid(), "ffmpeg_video: cannot create aac encoder context");
    output_handle_.audio_enc_ctx_.setCodec(output_handle_.audio_codec_);
    output_handle_.audio_enc_ctx_.setSampleRate(48000);
    output_handle_.audio_enc_ctx_.setChannels(2);
    // aac 编解码器必然支持立体声
    output_handle_.audio_enc_ctx_.setChannelLayout(av::ChannelLayout{AV_CH_LAYOUT_STEREO});

    output_handle_.audio_enc_ctx_.setSampleFormat(pick_first_supported_sample_fmt(
        output_handle_.audio_enc_ctx_.codec(), output_handle_.audio_enc_ctx_.sampleFormat()
    ));
    output_handle_.audio_enc_ctx_.setTimeBase(av::Rational{1, output_handle_.audio_enc_ctx_.sampleRate()});
    output_handle_.audio_enc_ctx_.open();

    output_handle_.audio_stream_ = output_handle_.format_context_.addStream(output_handle_.audio_enc_ctx_);
    output_handle_.audio_stream_.setTimeBase(output_handle_.audio_enc_ctx_.timeBase());
    output_handle_.audio_next_pts_ = av::Timestamp{0, output_handle_.audio_stream_.timeBase()};
  }

 public:
  void open(const FSys::path& in_path, const FSys::path& out_path) {
    open_input_video(in_path);
    open_output_video(out_path);
  }

  // 添加音频轨道, 传入 MP4 文件路径, 提取音频轨道, 检查必须为 AAC 编码, 并将流直接复制到输出文件
  void add_audio(const FSys::path& in_audio_path) {
    open_output_audio();

    audio_handle_.open_format_context(in_audio_path);
    audio_handle_.open_audio_context();
    audio_handle_.add_audio_resampler(output_handle_.audio_enc_ctx_);
  }

  /// @warning 仅支持 .srt 字幕文件, 并且必须在添加片头, 集数名称, 片尾 之后调用, 因为字幕需要知道最终的视频尺寸,
  /// 和时间偏移计算
  void add_subtitle(const FSys::path& in_subtitle_path, bool add_time_code, const std::string add_watermark) {
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
    const auto tb     = output_handle_.video_next_pts_.timebase();
    auto sar          = input_video_handle_.video_stream_.sampleAspectRatio();
    if (sar == av::Rational{}) sar = av::Rational{1, 1};

    // 计算时间偏移
    av::Timestamp total_offset_frames{0, output_handle_.video_next_pts_.timebase()};
    if (intro_handle_.video_stream_.isValid()) total_offset_frames += intro_handle_.video_stream_.duration();

    if (episodes_name_handle_.video_stream_.isValid())
      total_offset_frames += episodes_name_handle_.video_stream_.duration();

    const std::string buffer_args = std::format(
        "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}", w, h, pix_fmt, tb.getNumerator(),
        tb.getDenominator(), sar.getNumerator(), sar.getDenominator()
    );

    subtitle_handle_ = std::make_unique<subtitle_handle_t>();
    subtitle_handle_->graph_.setAutoConvert(AVFILTER_AUTO_CONVERT_ALL);

    const av::Filter buffer_filter{"buffer"};
    const av::Filter buffersink_filter{"buffersink"};
    const av::Filter subtitles_filter{"subtitles"};
    DOODLE_CHICK(buffer_filter, "ffmpeg_video: cannot find filter 'buffer'");
    DOODLE_CHICK(buffersink_filter, "ffmpeg_video: cannot find filter 'buffersink'");
    DOODLE_CHICK(subtitles_filter, "ffmpeg_video: cannot find filter 'subtitles' (FFmpeg needs libass)");

    subtitle_handle_->buffersrc_ctx_  = subtitle_handle_->graph_.createFilter(buffer_filter, "in", buffer_args);
    subtitle_handle_->buffersink_ctx_ = subtitle_handle_->graph_.createFilter(buffersink_filter, "out", "");

    {  // subtitles 的 filename 在 Windows 上包含 ':'，直接走 args 字符串容易被 ':' 分隔符影响。
      // 这里用 av_opt_set 设置 option，再 init。
      subtitle_handle_->subtitles_ctx_ = subtitle_handle_->graph_.allocFilter(subtitles_filter, "sub");
      const std::string subtitle_file  = in_subtitle_path.generic_string();
      {
        const int ret = av_opt_set(
            subtitle_handle_->subtitles_ctx_.raw(), "filename", subtitle_file.c_str(), AV_OPT_SEARCH_CHILDREN
        );
        DOODLE_CHICK(ret >= 0, std::format("ffmpeg_video: set subtitles filename failed: {}", subtitle_file));
      }
      // 额外添加可选的位移参数
      subtitle_handle_->subtitles_ctx_.init(
          total_offset_frames.timestamp() == 0 ? "" : std::format("sub_shift={}", total_offset_frames.seconds())
      );
    }
    if (add_time_code) {
      // 时间码 使用 drawtext 烧录, 格式 HH:MM:SS:FF, 文字位于右上角
      const av::Filter timecode_filter{"drawtext"};
      DOODLE_CHICK(timecode_filter, "ffmpeg_video: cannot find filter 'drawtext'");
      const std::string timecode_args = std::format(
          R"(timecode='00\:00\:00\:00':r={}:x=1585:y=107:fontcolor=white:fontsize=80:borderw=5:bordercolor=black)",
          g_fps
      );
      subtitle_handle_->timecode_ctx_ = subtitle_handle_->graph_.createFilter(timecode_filter, "tc", timecode_args);
    }
    if (!add_watermark.empty()) {
      // 水印过滤器 使用 drawtext 烧录, 文字位于左上角
      const av::Filter watermark_filter{"drawtext"};
      DOODLE_CHICK(watermark_filter, "ffmpeg_video: cannot find filter 'drawtext'");
      const std::string watermark_args = std::format(
          R"(text='{}':x=105:y=136:fontcolor=white:fontsize=80:borderw=5:bordercolor=black)", add_watermark
      );
      subtitle_handle_->watermark_ctx_ = subtitle_handle_->graph_.createFilter(watermark_filter, "wm", watermark_args);
    }
    subtitle_handle_->buffersrc_ctx_.link(0, subtitle_handle_->subtitles_ctx_, 0);
    av::FilterContext* last_ctx = &subtitle_handle_->subtitles_ctx_;

    if (add_time_code) {
      // 连接时间码过滤器
      last_ctx->link(0, subtitle_handle_->timecode_ctx_, 0);
      last_ctx = &subtitle_handle_->timecode_ctx_;
    }
    if (!add_watermark.empty()) {
      // 连接水印过滤器
      last_ctx->link(0, subtitle_handle_->watermark_ctx_, 0);
      last_ctx = &subtitle_handle_->watermark_ctx_;
    }
    last_ctx->link(0, subtitle_handle_->buffersink_ctx_, 0);

    subtitle_handle_->graph_.config();
    subtitle_handle_->buffersrc_  = av::BufferSrcFilterContext{subtitle_handle_->buffersrc_ctx_};
    subtitle_handle_->buffersink_ = av::BufferSinkFilterContext{subtitle_handle_->buffersink_ctx_};
    subtitle_handle_->configured_ = true;
  }

  void add_episodes_name(const FSys::path& in_episodes_name_path) {
    episodes_name_handle_.open_format_context(in_episodes_name_path);
    episodes_name_handle_.open_video_context();
  }
  void add_intro_outro(const FSys::path& in_intro_path, const FSys::path& in_outro_path) {
    intro_handle_.open_format_context(in_intro_path);
    intro_handle_.open_video_context();
    intro_handle_.open_audio_context();
    intro_handle_.add_audio_resampler(output_handle_.audio_enc_ctx_);

    outro_handle_.open_format_context(in_outro_path);
    outro_handle_.open_video_context();
    outro_handle_.open_audio_context();
    outro_handle_.add_audio_resampler(output_handle_.audio_enc_ctx_);
  }

  void encode_video_frame(av::VideoFrame& in_frame) {
    in_frame.setTimeBase(output_handle_.video_next_pts_.timebase());
    in_frame.setPts(output_handle_.video_next_pts_);
    output_handle_.video_next_pts_ += av::Timestamp{1, output_handle_.video_next_pts_.timebase()};
    if (auto out_pkt = output_handle_.video_enc_ctx_.encode(in_frame); out_pkt) {
      out_pkt.setTimeBase(output_handle_.video_next_pts_.timebase());
      out_pkt.setStreamIndex(output_handle_.video_stream_.index());
      output_handle_.format_context_.writePacket(out_pkt);
    }
  }
  void encode_audio_frame(av::AudioSamples& in_frame) {
    in_frame.setTimeBase(output_handle_.audio_next_pts_.timebase());
    in_frame.setPts(output_handle_.audio_next_pts_);
    output_handle_.audio_next_pts_ += av::Timestamp{in_frame.samplesCount(), in_frame.timeBase()};

    if (auto out_pkt = output_handle_.audio_enc_ctx_.encode(in_frame); out_pkt) {
      out_pkt.setTimeBase(output_handle_.audio_stream_.timeBase());
      out_pkt.setStreamIndex(output_handle_.audio_stream_.index());
      output_handle_.format_context_.writePacket(out_pkt);
    }
  }
  void flush_video_encoder() {
    const int l_out_video_index = output_handle_.video_stream_.index();
    // Flush video encoder
    while (auto out_pkt = output_handle_.video_enc_ctx_.encode()) {
      out_pkt.setTimeBase(output_handle_.video_next_pts_.timebase());
      out_pkt.setStreamIndex(l_out_video_index);
      output_handle_.format_context_.writePacket(out_pkt);
    }
  }
  void flush_audio_encoder() {
    const int l_out_audio_index = output_handle_.audio_stream_.index();
    // Flush audio encoder
    while (auto out_pkt = output_handle_.audio_enc_ctx_.encode()) {
      out_pkt.setTimeBase(output_handle_.audio_next_pts_.timebase());
      out_pkt.setStreamIndex(l_out_audio_index);
      output_handle_.format_context_.writePacket(out_pkt);
    }
  }
  void flush_subtitle_filter() {
    if (subtitle_handle_ && subtitle_handle_->configured_) {
      (void)av_buffersrc_add_frame_flags(subtitle_handle_->buffersrc_ctx_.raw(), nullptr, 0);
      av::VideoFrame filtered;
      while (subtitle_handle_->buffersink_.getVideoFrame(filtered)) encode_video_frame(filtered);
    }
  }

  void process() {
    output_handle_.format_context_.writeHeader();
    if (intro_handle_.video_stream_.isValid()) {
      intro_handle_.process_output_video(*this);
    }
    if (episodes_name_handle_.video_stream_.isValid()) {
      episodes_name_handle_.process_output_video(*this);
    }
    // 处理主视频流
    input_video_handle_.process_output_video(*this);
    if (outro_handle_.video_stream_.isValid()) {
      outro_handle_.process_output_video(*this);
    }
    flush_video_encoder();
    flush_subtitle_filter();
    if (output_handle_.audio_stream_.isValid()) {
      if (intro_handle_.audio_stream_.isValid()) {
        intro_handle_.process_output_audio(*this);
      }
      if (episodes_name_handle_.video_stream_.isValid()) {
        // 这个 mp4 文件没有音频流, 不处理, 但是要偏移音频流的 时间戳
        output_handle_.audio_next_pts_ += episodes_name_handle_.video_stream_.duration();
      }
      if (!audio_handle_.format_context_.isNull()) {
        audio_handle_.process_output_audio(*this);
      }
      if (outro_handle_.audio_stream_.isValid()) {
        outro_handle_.process_output_audio(*this);
      }
      flush_audio_encoder();
    }

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
  if (!audio_path_.empty() && FSys::exists(audio_path_)) {
    impl_->add_audio(audio_path_);
  }
  if (!episodes_name_path_.empty() && FSys::exists(episodes_name_path_)) {
    impl_->add_episodes_name(episodes_name_path_);
  }
  if (!intro_path_.empty() && FSys::exists(intro_path_) && !outro_path_.empty() && FSys::exists(outro_path_)) {
    impl_->add_intro_outro(intro_path_, outro_path_);
  }
  if (!subtitle_path_.empty() && FSys::exists(subtitle_path_)) {
    impl_->add_subtitle(subtitle_path_, time_code_, watermark_text_);
  }

  impl_->process();
}

void ffmpeg_video::check_video_valid(const FSys::path& in_video_path, bool has_video_stream) {
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
  if (has_video_stream) {
    DOODLE_CHICK(l_in_video_stream.isValid(), "ffmpeg_video: input has no video stream");

    // video decoder must exist
    av::Codec l_in_video_codec = l_in_video_stream.codecParameters().decodingCodec();
    DOODLE_CHICK(!l_in_video_codec.isNull(), "ffmpeg_video: cannot find video decoder");
    DOODLE_CHICK(l_in_video_codec.canDecode(), "ffmpeg_video: video decoder cannot decode");
  }

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