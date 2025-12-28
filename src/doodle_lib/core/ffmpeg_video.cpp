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
#include <memory>
#include <optional>
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

  av::FormatContext input_format_context_;
  av::FormatContext output_format_context_;
  av::Stream in_video_stream_;
  av::Stream out_video_stream_;

  //
  av::Codec in_video_codec_;
  av::Codec h264_codec_;

  av::VideoDecoderContext in_video_dec_ctx_;
  av::VideoEncoderContext out_video_enc_ctx_;

  // 转换器
  av::VideoRescaler video_rescaler_;
  // 视频长度
  av::Timestamp video_duration_{};

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
    av::AudioEncoderContext enc_ctx_;
    av::AudioDecoderContext dec_ctx_;
    av::AudioResampler resampler_;
    av::Stream out_stream_;
  } audio_handle_;
  constexpr static int g_fps = 25;

  const av::Rational& get_video_time_base() const {
    static const av::Rational l_video_tb{1, g_fps};
    return l_video_tb;
  }

  void open(const FSys::path& in_path, const FSys::path& out_path) {
    input_format_context_.openInput(in_path.string());
    input_format_context_.findStreamInfo();

    // 获取视频长度
    video_duration_ = input_format_context_.duration();

    output_format_context_.openOutput(out_path.string());

    for (size_t i = 0; i < input_format_context_.streamsCount(); ++i) {
      auto st = input_format_context_.stream(i);
      if (st.isVideo()) {
        in_video_stream_ = st;
        break;
      }
    }
    DOODLE_CHICK(in_video_stream_.isValid(), "ffmpeg_video: input has no video stream");

    in_video_codec_ = in_video_stream_.codecParameters().decodingCodec();
    DOODLE_CHICK(!in_video_codec_.isNull(), "ffmpeg_video: cannot find video decoder");
    DOODLE_CHICK(in_video_codec_.isDecoder(), "ffmpeg_video: video decoder is not decoder");
    h264_codec_ = av::findEncodingCodec(AV_CODEC_ID_H264);
    DOODLE_CHICK(h264_codec_.canEncode(), "ffmpeg_video: cannot find H264 encoder");

    in_video_dec_ctx_ = av::VideoDecoderContext{in_video_stream_, in_video_codec_};
    in_video_dec_ctx_.open();

    constexpr static int k_fps = 25;
    const static av::Rational l_video_tb{1, k_fps};
    out_video_enc_ctx_ = av::VideoEncoderContext{h264_codec_};
    out_video_enc_ctx_.setWidth(in_video_dec_ctx_.width());
    out_video_enc_ctx_.setHeight(in_video_dec_ctx_.height());
    out_video_enc_ctx_.setTimeBase(l_video_tb);
    out_video_enc_ctx_.setPixelFormat(pick_first_supported_pix_fmt(h264_codec_, in_video_dec_ctx_.pixelFormat()));
    out_video_enc_ctx_.open();

    out_video_stream_ = output_format_context_.addStream(out_video_enc_ctx_);
    out_video_stream_.setTimeBase(l_video_tb);
    out_video_stream_.setFrameRate(av::Rational{k_fps, 1});
    out_video_stream_.setAverageFrameRate(av::Rational{k_fps, 1});

    if (in_video_dec_ctx_.width() != out_video_enc_ctx_.width() ||
        in_video_dec_ctx_.height() != out_video_enc_ctx_.height() ||
        in_video_dec_ctx_.pixelFormat() != out_video_enc_ctx_.pixelFormat()) {
      video_rescaler_ =
          av::VideoRescaler{out_video_enc_ctx_.width(), out_video_enc_ctx_.height(), out_video_enc_ctx_.pixelFormat(),
                            in_video_dec_ctx_.width(),  in_video_dec_ctx_.height(),  in_video_dec_ctx_.pixelFormat()};
    }
  }

  void add_audio(const FSys::path& in_audio_path) {
    audio_handle_.format_context_.openInput(in_audio_path.string());
    audio_handle_.format_context_.findStreamInfo();

    // 获取音频长度并进行检查
    auto l_audio_duration = audio_handle_.format_context_.duration();
    DOODLE_CHICK(
        l_audio_duration == video_duration_,
        std::format(
            "ffmpeg_video: audio duration {} does not match video duration {}", l_audio_duration, video_duration_
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

    audio_handle_.codec_ = audio_handle_.stream_.codecParameters().decodingCodec();
    DOODLE_CHICK(!audio_handle_.codec_.isNull(), "ffmpeg_video: cannot find audio decoder");
    DOODLE_CHICK(audio_handle_.codec_.canDecode(), "ffmpeg_video: cannot find audio decoder");

    audio_handle_.dec_ctx_ = av::AudioDecoderContext{audio_handle_.stream_, audio_handle_.codec_};
    audio_handle_.dec_ctx_.open();

    audio_handle_.enc_ctx_ = av::AudioEncoderContext{};
    audio_handle_.enc_ctx_.setCodec(av::findEncodingCodec(AV_CODEC_ID_AAC));

    const int l_dst_sample_rate = audio_handle_.dec_ctx_.sampleRate() > 0 ? audio_handle_.dec_ctx_.sampleRate() : 48000;
    const std::uint64_t l_src_channel_layout = audio_handle_.dec_ctx_.channelLayout() != 0
                                                   ? audio_handle_.dec_ctx_.channelLayout()
                                                   : av_get_default_channel_layout(audio_handle_.dec_ctx_.channels());

    const uint64_t l_dst_layout = pick_channel_layout(l_src_channel_layout, audio_handle_.enc_ctx_.codec());
    const av::SampleFormat l_dst_sample_fmt =
        pick_first_supported_sample_fmt(audio_handle_.enc_ctx_.codec(), av::SampleFormat{AV_SAMPLE_FMT_FLTP});

    audio_handle_.enc_ctx_.setSampleRate(l_dst_sample_rate);
    audio_handle_.enc_ctx_.setChannelLayout(l_dst_layout);
    audio_handle_.enc_ctx_.setSampleFormat(l_dst_sample_fmt);
    const av::Rational l_audio_tb{1, audio_handle_.enc_ctx_.sampleRate()};
    audio_handle_.enc_ctx_.setTimeBase(l_audio_tb);
    audio_handle_.enc_ctx_.open();

    audio_handle_.out_stream_ = output_format_context_.addStream(audio_handle_.enc_ctx_);
    audio_handle_.out_stream_.setTimeBase(l_audio_tb);

    if (audio_handle_.dec_ctx_.sampleRate() != audio_handle_.enc_ctx_.sampleRate() ||
        audio_handle_.dec_ctx_.channelLayout() != audio_handle_.enc_ctx_.channelLayout() ||
        audio_handle_.dec_ctx_.sampleFormat() != audio_handle_.enc_ctx_.sampleFormat()) {
      audio_handle_.resampler_.init(
          audio_handle_.enc_ctx_.channelLayout(), audio_handle_.enc_ctx_.sampleRate(),
          audio_handle_.enc_ctx_.sampleFormat(), l_src_channel_layout, audio_handle_.dec_ctx_.sampleRate(),
          audio_handle_.dec_ctx_.sampleFormat()
      );
    }
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

    const auto w = out_video_enc_ctx_.width();
    const auto h = out_video_enc_ctx_.height();
    DOODLE_CHICK(w > 0 && h > 0, "ffmpeg_video: invalid video size for subtitle graph");

    const int pix_fmt = static_cast<int>(out_video_enc_ctx_.pixelFormat());
    const auto tb     = get_video_time_base();
    auto sar          = in_video_stream_.sampleAspectRatio();
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

  void process_out_video() {
    const int l_out_video_index = out_video_stream_.index();

    // -----------------
    // Encode video packets
    // -----------------
    int64_t l_video_frame_index = 0;
    while (auto pkt_opt = read_next_packet_for_stream(input_format_context_, in_video_stream_.index())) {
      auto frame = in_video_dec_ctx_.decode(*pkt_opt);
      if (!frame) {
        continue;
      }

      auto encode_and_write = [&](av::VideoFrame& in_frame) {
        auto out_pkt = out_video_enc_ctx_.encode(in_frame);
        if (out_pkt && !out_pkt.isNull()) {
          out_pkt.setTimeBase(get_video_time_base());
          out_pkt.setStreamIndex(l_out_video_index);
          output_format_context_.writePacket(out_pkt);
        }
      };

      if (video_rescaler_.isValid()) {
        auto dst = video_rescaler_.rescale(frame);
        dst.setTimeBase(get_video_time_base());
        dst.setPts(av::Timestamp{l_video_frame_index++, get_video_time_base()});

        if (subtitle_handle_ && subtitle_handle_->configured_) {
          subtitle_handle_->buffersrc_.writeVideoFrame(dst);
          while (true) {
            av::VideoFrame filtered;
            if (!subtitle_handle_->buffersink_.getVideoFrame(filtered)) break;
            filtered.setTimeBase(get_video_time_base());
            encode_and_write(filtered);
          }
        } else {
          encode_and_write(dst);
        }
      } else {
        frame.setTimeBase(get_video_time_base());
        frame.setPts(av::Timestamp{l_video_frame_index++, get_video_time_base()});

        if (subtitle_handle_ && subtitle_handle_->configured_) {
          subtitle_handle_->buffersrc_.writeVideoFrame(frame);
          while (true) {
            av::VideoFrame filtered;
            if (!subtitle_handle_->buffersink_.getVideoFrame(filtered)) break;
            filtered.setTimeBase(get_video_time_base());
            encode_and_write(filtered);
          }
        } else {
          encode_and_write(frame);
        }
      }
    }

    // Flush subtitle filter
    if (subtitle_handle_ && subtitle_handle_->configured_) {
      (void)av_buffersrc_add_frame_flags(subtitle_handle_->buffersrc_ctx_.raw(), nullptr, 0);
      while (true) {
        av::VideoFrame filtered;
        if (!subtitle_handle_->buffersink_.getVideoFrame(filtered)) break;
        filtered.setTimeBase(get_video_time_base());
        auto out_pkt = out_video_enc_ctx_.encode(filtered);
        if (out_pkt && !out_pkt.isNull()) {
          out_pkt.setTimeBase(get_video_time_base());
          out_pkt.setStreamIndex(l_out_video_index);
          output_format_context_.writePacket(out_pkt);
        }
      }
    }

    // Flush video encoder
    while (true) {
      auto out_pkt = out_video_enc_ctx_.encode();
      if (!out_pkt || out_pkt.isNull()) {
        break;
      }
      out_pkt.setTimeBase(get_video_time_base());
      out_pkt.setStreamIndex(l_out_video_index);
      output_format_context_.writePacket(out_pkt);
    }
  }

  void process_audio() {
    // -----------------
    // Encode audio packets
    // -----------------
    const av::Rational l_audio_tb{1, audio_handle_.enc_ctx_.sampleRate()};
    const int l_out_audio_index  = audio_handle_.out_stream_.index();

    const int l_audio_frame_size = audio_handle_.enc_ctx_.frameSize() > 0 ? audio_handle_.enc_ctx_.frameSize() : 1024;
    int64_t l_audio_samples_written = 0;

    auto encode_audio_samples       = [&](av::AudioSamples& samples) {
      samples.setTimeBase(l_audio_tb);
      samples.setPts(av::Timestamp{l_audio_samples_written, l_audio_tb});
      l_audio_samples_written += samples.samplesCount();
      auto out_pkt = audio_handle_.enc_ctx_.encode(samples);
      if (out_pkt && !out_pkt.isNull()) {
        out_pkt.setTimeBase(l_audio_tb);
        out_pkt.setStreamIndex(l_out_audio_index);
        output_format_context_.writePacket(out_pkt);
      }
    };

    while (auto pkt_opt = read_next_packet_for_stream(audio_handle_.format_context_, audio_handle_.stream_.index())) {
      auto samples = audio_handle_.dec_ctx_.decode(*pkt_opt);
      if (!samples) {
        continue;
      }

      // avcpp may report channelsLayout() as 0 with FFmpeg new channel layout API
      // (e.g. when ch_layout.order is not AV_CHANNEL_ORDER_NATIVE). AudioResampler
      // requires a stable, non-zero layout mask, so we synthesize a default one.
      if (samples.channelsLayout() == 0) {
        int channels = samples.channelsCount();
        if (channels <= 0) {
          channels = audio_handle_.dec_ctx_.channels();
        }
        if (channels <= 0) {
          channels = 2;
        }
        av::frame::set_channel_layout(samples.raw(), av_get_default_channel_layout(channels));
      }
      if (samples.sampleRate() <= 0) {
        av::frame::set_sample_rate(samples.raw(), audio_handle_.dec_ctx_.sampleRate());
      }

      if (audio_handle_.resampler_.isValid()) {
        audio_handle_.resampler_.push(samples);
        while (true) {
          auto out = audio_handle_.resampler_.pop(static_cast<size_t>(l_audio_frame_size));
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
    if (audio_handle_.resampler_.isValid()) {
      // 为 0 时会一次提取所有样本, 不需要循环
      auto out = audio_handle_.resampler_.pop(0);
      if (out) encode_audio_samples(out);
    }

    // Flush audio encoder
    while (true) {
      auto out_pkt = audio_handle_.enc_ctx_.encode();
      if (!out_pkt || out_pkt.isNull()) {
        break;
      }
      out_pkt.setTimeBase(l_audio_tb);
      out_pkt.setStreamIndex(l_out_audio_index);
      output_format_context_.writePacket(out_pkt);
    }
  }

  // 处理字幕
  void process_subtitle() {}

  void process() {
    output_format_context_.writeHeader();
    process_out_video();
    if (!audio_handle_.format_context_.isNull()) {
      process_audio();
    }

    output_format_context_.writeTrailer();
  }
};

ffmpeg_video::ffmpeg_video(const FSys::path& in_video_path, const FSys::path& in_out_path)
    : impl_(std::make_unique<impl>()), video_path_(in_video_path), out_path_(in_out_path) {}

ffmpeg_video::~ffmpeg_video() = default;

void ffmpeg_video::process() {
  out_path_.replace_extension(".mp4");
  DOODLE_CHICK(!video_path_.empty() && FSys::exists(video_path_), "ffmpeg_video: video path is empty or not exists");

  impl_->open(video_path_, out_path_);
  if (!subtitle_path_.empty()) {
    impl_->add_subtitle(subtitle_path_);
  }
  if (!audio_path_.empty()) {
    impl_->add_audio(audio_path_);
  }
  impl_->process();
}

void ffmpeg_video::preprocess_wav_to_aac(const FSys::path& in_wav_path, const FSys::path& in_out_path) {
  DOODLE_CHICK(!in_wav_path.empty() && FSys::exists(in_wav_path), "ffmpeg_video: audio path is empty or not exists");
  DOODLE_CHICK(!in_out_path.empty(), "ffmpeg_video: output path is empty");

  av::FormatContext l_input_ctx{};
  l_input_ctx.openInput(in_wav_path.string());
  l_input_ctx.findStreamInfo();

  av::FormatContext l_output_ctx{};
  l_output_ctx.openOutput(in_out_path.string());

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

}  // namespace doodle