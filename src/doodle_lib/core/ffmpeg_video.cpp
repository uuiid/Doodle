#include "ffmpeg_video.h"

#include "doodle_core/configure/static_value.h"
#include "doodle_core/core/global_function.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/logger/logger.h"

#include <boost/numeric/conversion/cast.hpp>

#include <algorithm>
#include <avcpp/audioresampler.h>
#include <avcpp/codec.h>
#include <avcpp/codeccontext.h>
#include <avcpp/filtergraph.h>
#include <avcpp/formatcontext.h>
#include <avcpp/frame.h>
#include <avcpp/stream.h>
#include <avcpp/timestamp.h>
#include <avcpp/videorescaler.h>
#include <averror.h>
#include <channellayout.h>
#include <cstdint>
#include <ffmpeg.h>
#include <filesystem>
#include <filtercontext.h>
#include <fmt/format.h>
#include <libavcodec/codec_id.h>
#include <libavformat/avformat.h>
#include <memory>
#include <packet.h>
#include <rational.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <system_error>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
}

namespace doodle {

namespace {

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

class AVAudioFifoWarp : FFWrapperPtr<AVAudioFifo> {
  std::int32_t channels_{0};
  av::SampleFormat sample_format_{};

 public:
  using FFWrapperPtr<AVAudioFifo>::FFWrapperPtr;

  bool isValid() const noexcept { return m_raw != nullptr; }

  void init(av::SampleFormat sample_format, int channels, int nb_samples, av::OptionalErrorCode ec = av::throws()) {
    clear_if(ec);
    if (isValid()) {
      av_audio_fifo_free(m_raw);
      m_raw = nullptr;
    }
    sample_format_ = sample_format;
    channels_      = channels;
    m_raw          = av_audio_fifo_alloc(sample_format, channels, nb_samples);
    if (m_raw == nullptr) {
      fflog(AV_LOG_FATAL, "Can't alloc AVAudioFifo\n");
      throws_if(ec, ENOMEM, std::system_category());
    }
  }

  int write(const av::AudioSamples& frame, av::OptionalErrorCode ec = av::throws()) {
    clear_if(ec);
    if (!isValid()) {
      fflog(AV_LOG_ERROR, "AVAudioFifo is not initialized\n");
      throws_if(ec, ENOMEM, std::system_category());
      return -1;
    }
    int sts = av_audio_fifo_write(m_raw, reinterpret_cast<void**>(frame.raw()->extended_data), frame.samplesCount());
    if (sts < 0) {
      fflog(AV_LOG_ERROR, "Can't write data to AVAudioFifo\n");
      throws_if(ec, sts, av::ffmpeg_category());
    }
    return sts;
  }

  av::AudioSamples read(int nb_samples, std::int32_t sampl_rate, av::OptionalErrorCode ec = av::throws()) {
    clear_if(ec);
    av::AudioSamples frame{};
    if (!isValid()) {
      fflog(AV_LOG_ERROR, "AVAudioFifo is not initialized\n");
      throws_if(ec, ENOMEM, std::system_category());
      return frame;
    }

    nb_samples = std::min(nb_samples, samplesAvailable());
    if (nb_samples <= 0) {
      return frame;
    }

    frame.init(sample_format_, nb_samples, av_get_default_channel_layout(channels_), sampl_rate);
    int sts = av_audio_fifo_read(m_raw, reinterpret_cast<void**>(frame.raw()->extended_data), nb_samples);
    if (sts < 0) {
      fflog(AV_LOG_ERROR, "Can't read data from AVAudioFifo\n");
      throws_if(ec, sts, av::ffmpeg_category());
      return av::AudioSamples{};
    }
    DOODLE_CHICK(sts == nb_samples, "ffmpeg_video: AVAudioFifo read less samples than requested");

    return frame;
  }

  int samplesAvailable() const noexcept {
    if (!isValid()) {
      return 0;
    }
    return av_audio_fifo_size(m_raw);
  }

  operator bool() const noexcept { return isValid(); }
};

constexpr static int g_fps = 25;
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
      if (out_codec.channelLayout2().layout() != audio_channel_layout_.layout() ||
          out_codec.sampleRate() != audio_dec_ctx_.sampleRate() ||
          out_codec.sampleFormat() != audio_dec_ctx_.sampleFormat())
        audio_resampler_.init(
            out_codec.channelLayout2().layout(), out_codec.sampleRate(), out_codec.sampleFormat(),
            audio_channel_layout_.layout(), audio_dec_ctx_.sampleRate(), audio_dec_ctx_.sampleFormat()
        );
    }

    void process(ffmpeg_video::impl& parent, bool add_audio_silence) {
      auto l_channel_layout_fixed = audio_channel_layout_ == audio_dec_ctx_.channelLayout2();
      while (auto l_pkt = format_context_.readPacket()) {
        if (l_pkt.streamIndex() == video_stream_.index()) process_output_video(parent, l_pkt);
        if (l_pkt.streamIndex() == audio_stream_.index()) process_output_audio(parent, l_channel_layout_fixed, l_pkt);
      }
      flush_audio_decoder(parent, l_channel_layout_fixed);
      flush_video_decoder(parent);
      flush_audio_resampler(parent);
      if (!audio_stream_.isValid() && add_audio_silence) process_until_video_pts(parent);
    }
    // 由于音频的特殊性质, 需要将视频中没有音频的部分补全静音音频
    void process_until_video_pts(ffmpeg_video::impl& parent) {
      // 首先计算需要补全多少音频采样（用 int64 rescale，避免长视频时的 32-bit 溢出）
      const auto l_dur                 = video_stream_.duration();
      const int l_sample_rate          = parent.output_handle_.audio_enc_ctx_.sampleRate();
      std::int64_t l_nb_samples_needed = l_dur.timebase().rescale(l_dur.timestamp(), av::Rational{1, l_sample_rate});
      const std::int64_t l_frame_size_ = parent.output_handle_.audio_enc_ctx_.frameSize();
      while (l_nb_samples_needed > 0) {
        auto l_framesize = l_nb_samples_needed > l_frame_size_ ? l_frame_size_ : l_nb_samples_needed;

        av::AudioSamples l_frame{};
        l_frame.init(
            parent.output_handle_.audio_enc_ctx_.sampleFormat(), l_framesize,
            parent.output_handle_.audio_enc_ctx_.channelLayout2().layout(),
            parent.output_handle_.audio_enc_ctx_.sampleRate()
        );

        // AudioSamples::init() 只分配 buffer，不保证清零。
        // 对于 flt/fltp 等浮点采样格式，未初始化数据可能包含 NaN/Inf，AAC 编码器会直接返回 EINVAL。
        (void)av_samples_set_silence(
            l_frame.raw()->extended_data, 0, static_cast<int>(l_framesize),
            parent.output_handle_.audio_enc_ctx_.channels(), static_cast<AVSampleFormat>(l_frame.raw()->format)
        );

        l_frame.setTimeBase(parent.output_handle_.audio_enc_ctx_.timeBase());
        parent.submit_audio_frame(l_frame);  // 编码静音音频帧
        l_nb_samples_needed -= l_framesize;
      }
    }

   private:
    void process_output_video(ffmpeg_video::impl& in_parent, av::Packet& in_pkt) {
      auto frame = video_dec_ctx_.decode(in_pkt);
      if (!frame) return;

      in_parent.process_filter(frame);
    }
    void process_output_audio(ffmpeg_video::impl& parent, bool in_channel_layout_fixed, av::Packet& in_pkt) {
      auto frame = audio_dec_ctx_.decode(in_pkt);
      if (!frame) return;

      process_audio_frame(parent, in_channel_layout_fixed, frame);
    }
    // 处理音频帧
    void process_audio_frame(ffmpeg_video::impl& parent, bool in_channel_layout_fixed, av::AudioSamples& in_frame) {
      if (!in_channel_layout_fixed) {
        av::frame::set_channel_layout(in_frame.raw(), audio_channel_layout_.layout());
      }
      if (in_frame.sampleRate() <= 0) av::frame::set_sample_rate(in_frame.raw(), audio_dec_ctx_.sampleRate());

      if (audio_resampler_.isValid()) {
        in_frame.setTimeBase(audio_dec_ctx_.timeBase());
        in_frame.setPts(parent.output_handle_.audio_next_pts_);
        audio_resampler_.push(in_frame);
        while (auto resampled_frame = audio_resampler_.pop(parent.output_handle_.audio_enc_ctx_.frameSize()))
          parent.submit_audio_frame(resampled_frame);
      } else
        parent.submit_audio_frame(in_frame);
    }

    void flush_audio_decoder(ffmpeg_video::impl& parent, bool in_channel_layout_fixed) {
      if (!audio_dec_ctx_.isValid()) return;
      while (auto l_frame = audio_dec_ctx_.decode({})) {
        process_audio_frame(parent, in_channel_layout_fixed, l_frame);
      }
    }
    void flush_video_decoder(ffmpeg_video::impl& parent) {
      if (!video_dec_ctx_.isValid()) return;
      while (auto l_frame = video_dec_ctx_.decode({})) {
        parent.process_filter(l_frame);
      }
    }

    // 将 av::AudioResampler 剩余的帧编码输出清空
    void flush_audio_resampler(ffmpeg_video::impl& parent) {
      if (audio_resampler_.isValid()) {
        // avcpp 的 AudioResampler::pop(0) 会按 swr_get_delay() 一次性分配所有剩余 samples。
        // 如果 swr_get_delay() 异常返回负数（转成 size_t 变成超大），会触发 CantAllocateFrame。
        // 这里改为：直接用 pop(dst, /*getall=*/true) 触发 flush，并用固定 frame_size 分块 drain，避免巨分配。
        const int frame_size = parent.output_handle_.audio_enc_ctx_.frameSize();
        DOODLE_CHICK(frame_size > 0, "ffmpeg_video: invalid audio encoder frame size");

        av::AudioSamples drained{};
        drained.init(
            parent.output_handle_.audio_enc_ctx_.sampleFormat(), frame_size,
            parent.output_handle_.audio_enc_ctx_.channelLayout2().layout(),
            parent.output_handle_.audio_enc_ctx_.sampleRate()
        );
        while (audio_resampler_.pop(drained, true)) {
          parent.submit_audio_frame(drained);
          // pop() 可能会把 nb_samples 改小；这里重置为期望的 frame_size，确保下一轮仍以固定块大小读取。
          drained = av::AudioSamples{};
          drained.init(
              parent.output_handle_.audio_enc_ctx_.sampleFormat(), frame_size,
              parent.output_handle_.audio_enc_ctx_.channelLayout2().layout(),
              parent.output_handle_.audio_enc_ctx_.sampleRate()
          );
        }
      }
    }
  };

  // 输出视频
  struct out_video : base_t {
    av::Codec h264_codec_;
    av::VideoEncoderContext video_enc_ctx_;

    av::Timestamp video_next_pts_{};
    // 视频流包时间
    av::Timestamp video_packet_time_{};
    // 音频流
    av::AudioEncoderContext audio_enc_ctx_;

    av::Timestamp audio_next_pts_{};
    av::Timestamp audio_packet_time_{};
  };
  out_video output_handle_;

  // 全局音频 FIFO：跨片头/正片/片尾拼接时，保证 AAC 始终按 frame_size(通常1024) 喂数据。
  AVAudioFifoWarp audio_fifo_{nullptr};

  // 输入视频
  base_t input_video_handle_;

  struct filter_handle_t {
    av::FilterGraph graph_{};
    av::FilterContext buffersrc_ctx_{};
    av::BufferSrcFilterContext buffersrc_{};

    av::FilterContext buffersink_ctx_{};
    av::BufferSinkFilterContext buffersink_{};
    bool configured_{false};

    void init_buffersrc(const av::VideoEncoderContext& dec_ctx) {
      auto l_time_base     = dec_ctx.timeBase();
      auto l_sample_aspect = dec_ctx.sampleAspectRatio();
      DOODLE_CHICK(l_sample_aspect != av::Rational{}, "ffmpeg_video: invalid sample aspect ratio");

      const std::string l_buffer_args = std::format(
          "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}", dec_ctx.width(), dec_ctx.height(),
          static_cast<int>(dec_ctx.pixelFormat()), l_time_base.getNumerator(), l_time_base.getDenominator(),
          l_sample_aspect.getNumerator(), l_sample_aspect.getDenominator()
      );
      graph_.setAutoConvert(AVFILTER_AUTO_CONVERT_ALL);

      const av::Filter buffer_filter{"buffer"};
      const av::Filter buffersink_filter{"buffersink"};

      buffersrc_ctx_  = graph_.createFilter(buffer_filter, "in", l_buffer_args);
      buffersink_ctx_ = graph_.createFilter(buffersink_filter, "out", "");
    }

    void configure() {
      graph_.config();
      buffersrc_  = av::BufferSrcFilterContext{buffersrc_ctx_};
      buffersink_ = av::BufferSinkFilterContext{buffersink_ctx_};
      configured_ = true;
    }
  };

  struct subtitle_handle_t : filter_handle_t {
    // 渲染字幕过滤器
    av::FilterContext subtitles_ctx_{};
    av::Timestamp time_offset_{0, {1, 1000}};
  };

  std::unique_ptr<subtitle_handle_t> subtitle_handle_;
  // 水印和时间码过滤器
  struct watermark_timecode_handle_t : filter_handle_t {
    // 时间码过滤器
    av::FilterContext timecode_ctx_{};
    // 水印过滤器
    av::FilterContext watermark_ctx_{};
  };
  std::unique_ptr<watermark_timecode_handle_t> watermark_timecode_handle_;

  // 音频组件
  base_t audio_handle_;

  // 集数文件(这是一个mp4文件，里面有视频轨道, 无音频轨道)
  base_t episodes_name_handle_;
  // 片头 包含一个视频流和一个音频流
  base_t intro_handle_;
  // 片尾 包含一个视频流和一个音频流
  base_t outro_handle_;

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

    const static av::Rational l_video_tb{1, g_fps};
    output_handle_.video_enc_ctx_ = av::VideoEncoderContext{output_handle_.h264_codec_};
    output_handle_.video_enc_ctx_.setWidth(input_video_handle_.video_dec_ctx_.width());
    output_handle_.video_enc_ctx_.setHeight(input_video_handle_.video_dec_ctx_.height());
    output_handle_.video_enc_ctx_.setTimeBase(l_video_tb);
    output_handle_.video_enc_ctx_.setPixelFormat(
        pick_first_supported_pix_fmt(output_handle_.h264_codec_, input_video_handle_.video_dec_ctx_.pixelFormat())
    );
    // 设置码率 vbr 目标 9.9 mpbs
    constexpr static int k_bitrate = 9'900'000;
    output_handle_.video_enc_ctx_.setBitRate(k_bitrate);
    output_handle_.video_enc_ctx_.setBitRateRange({k_bitrate / 2, k_bitrate * 3 / 2});
    // // 对 libx264：CRF 模式通过私有选项设置。bit_rate 不设置（或置 0）避免和 ABR 混用。
    // output_handle_.video_enc_ctx_.setBitRate(0);
    // output_handle_.video_enc_ctx_.setBitRateRange({0, 0});

    // output_handle_.video_enc_ctx_.setOption("crf", std::to_string(video_rate_control_.crf_));
    // if (!video_rate_control_.preset_.empty()) {
    //   output_handle_.video_enc_ctx_.setOption("preset", video_rate_control_.preset_);
    // }
    // // 可选：VBV 约束（用于限制瞬时码率/缓冲，便于对接带宽或播放器要求）。
    // if (video_rate_control_.vbv_maxrate_ > 0) {
    //   output_handle_.video_enc_ctx_.setOption("maxrate", std::to_string(video_rate_control_.vbv_maxrate_));
    // }
    // if (video_rate_control_.vbv_bufsize_ > 0) {
    //   output_handle_.video_enc_ctx_.setOption("bufsize", std::to_string(video_rate_control_.vbv_bufsize_));
    // }

    output_handle_.video_enc_ctx_.open();

    output_handle_.video_stream_ = output_handle_.format_context_.addStream(output_handle_.video_enc_ctx_);
    output_handle_.video_stream_.setTimeBase(l_video_tb);
    output_handle_.video_stream_.setFrameRate(av::Rational{g_fps, 1});
    output_handle_.video_stream_.setAverageFrameRate(av::Rational{g_fps, 1});

    output_handle_.video_next_pts_    = av::Timestamp{0, l_video_tb};
    output_handle_.video_packet_time_ = av::Timestamp{0, l_video_tb};
  }

  void open_output_audio() {
    output_handle_.audio_codec_ = av::findEncodingCodec(AV_CODEC_ID_AAC);
    DOODLE_CHICK(output_handle_.audio_codec_.isEncoder(), "ffmpeg_video: cannot find aac encoder");
    output_handle_.audio_enc_ctx_ = av::AudioEncoderContext{output_handle_.audio_codec_};
    DOODLE_CHICK(output_handle_.audio_enc_ctx_.isValid(), "ffmpeg_video: cannot create aac encoder context");
    output_handle_.audio_enc_ctx_.setCodec(output_handle_.audio_codec_);
    output_handle_.audio_enc_ctx_.setSampleRate(48000);
    // aac 编解码器必然支持立体声
    output_handle_.audio_enc_ctx_.setChannelLayout(av::ChannelLayout{2});

    output_handle_.audio_enc_ctx_.setSampleFormat(pick_first_supported_sample_fmt(
        output_handle_.audio_enc_ctx_.codec(), output_handle_.audio_enc_ctx_.sampleFormat()
    ));
    output_handle_.audio_enc_ctx_.setTimeBase(av::Rational{1, output_handle_.audio_enc_ctx_.sampleRate()});
    output_handle_.audio_enc_ctx_.open();

    output_handle_.audio_stream_ = output_handle_.format_context_.addStream(output_handle_.audio_enc_ctx_);
    output_handle_.audio_stream_.setTimeBase(output_handle_.audio_enc_ctx_.timeBase());
    output_handle_.audio_next_pts_    = av::Timestamp{0, output_handle_.audio_stream_.timeBase()};
    output_handle_.audio_packet_time_ = av::Timestamp{0, output_handle_.audio_stream_.timeBase()};

    audio_fifo_.init(output_handle_.audio_enc_ctx_.sampleFormat().get(), output_handle_.audio_enc_ctx_.channels(), 1);
  }

  void submit_audio_frame(av::AudioSamples& in_frame) {
    if (!output_handle_.audio_stream_.isValid() || !audio_fifo_) return;
    if (!in_frame.isValid() || in_frame.samplesCount() <= 0) return;

    audio_fifo_.write(in_frame);

    const int frame_size = output_handle_.audio_enc_ctx_.frameSize();
    DOODLE_CHICK(frame_size > 0, "ffmpeg_video: invalid audio encoder frame size");
    while (audio_fifo_.samplesAvailable() >= frame_size) {
      auto l_sp = audio_fifo_.read(frame_size, output_handle_.audio_enc_ctx_.sampleRate());
      encode_audio_frame(l_sp);
    }
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
  void add_subtitle(const FSys::path& in_subtitle_path) {
    DOODLE_CHICK(!in_subtitle_path.empty(), "字幕路径为空");
    DOODLE_CHICK(FSys::exists(in_subtitle_path), std::format("字幕文件不存在: {}", in_subtitle_path.string()));
    DOODLE_CHICK(
        FSys::is_regular_file(in_subtitle_path), std::format("字幕路径不是文件: {}", in_subtitle_path.string())
    );

    // 使用 avfilter 的 subtitles 滤镜渲染 .srt (依赖 FFmpeg 编译启用 libass)
    // 滤镜图: buffer -> subtitles -> buffersink

    subtitle_handle_               = std::make_unique<subtitle_handle_t>();
    subtitle_handle_->time_offset_ = av::Timestamp{0, output_handle_.video_next_pts_.timebase()};
    // 计算时间偏移
    if (intro_handle_.video_stream_.isValid()) subtitle_handle_->time_offset_ += intro_handle_.video_stream_.duration();

    if (episodes_name_handle_.video_stream_.isValid())
      subtitle_handle_->time_offset_ += episodes_name_handle_.video_stream_.duration();

    subtitle_handle_->init_buffersrc(output_handle_.video_enc_ctx_);

    const av::Filter subtitles_filter{"subtitles"};
    DOODLE_CHICK(subtitles_filter, "ffmpeg_video: cannot find filter 'subtitles' (FFmpeg needs libass)");

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
      subtitle_handle_->subtitles_ctx_.init({});
    }

    subtitle_handle_->buffersrc_ctx_.link(0, subtitle_handle_->subtitles_ctx_, 0);
    subtitle_handle_->subtitles_ctx_.link(0, subtitle_handle_->buffersink_ctx_, 0);

    subtitle_handle_->configure();
  }

  void add_time_code_watermark(bool add_time_code, const std::string add_watermark) {
    DOODLE_CHICK(add_time_code || !add_watermark.empty(), "ffmpeg_video: either time code or watermark must be added");
    watermark_timecode_handle_ = std::make_unique<watermark_timecode_handle_t>();
    watermark_timecode_handle_->init_buffersrc(output_handle_.video_enc_ctx_);
    if (add_time_code) {
      static const std::string_view l_font_path{"C:/Windows/Fonts/consola.ttf"};
      // 时间码 使用 drawtext 烧录, 格式 HH:MM:SS:FF, 文字位于右上角
      const av::Filter timecode_filter{"drawtext"};
      DOODLE_CHICK(timecode_filter, "ffmpeg_video: cannot find filter 'drawtext'");
      const std::string timecode_args = std::format(
          R"(timecode='00:00:00:00':rate={}:x=1285:y=107:fontcolor=white:fontsize=80:borderw=5:bordercolor=black)",
          g_fps
      );
      watermark_timecode_handle_->timecode_ctx_ = watermark_timecode_handle_->graph_.allocFilter(timecode_filter, "tc");
      // 添加字体路径
      const int ret                             = av_opt_set(
          watermark_timecode_handle_->timecode_ctx_.raw(), "fontfile", l_font_path.data(), AV_OPT_SEARCH_CHILDREN
      );
      DOODLE_CHICK(ret >= 0, std::format("ffmpeg_video: set timecode fontfile failed: {}", l_font_path));
      watermark_timecode_handle_->timecode_ctx_.init(timecode_args);
    }
    if (!add_watermark.empty()) {
      static const std::string_view l_font_path{"C:/Windows/Fonts/simhei.ttf"};
      // 水印过滤器 使用 drawtext 烧录, 文字位于左上角
      const av::Filter watermark_filter{"drawtext"};
      DOODLE_CHICK(watermark_filter, "ffmpeg_video: cannot find filter 'drawtext'");
      const std::string watermark_args = std::format(
          R"(text='{}':x=105:y=107:fontcolor=white:fontsize=80:borderw=5:bordercolor=black)", add_watermark
      );
      watermark_timecode_handle_->watermark_ctx_ =
          watermark_timecode_handle_->graph_.allocFilter(watermark_filter, "wm");
      // 添加字体路径
      const int ret = av_opt_set(
          watermark_timecode_handle_->watermark_ctx_.raw(), "fontfile", l_font_path.data(), AV_OPT_SEARCH_CHILDREN
      );
      DOODLE_CHICK(ret >= 0, std::format("ffmpeg_video: set watermark fontfile failed: {}", l_font_path));
      watermark_timecode_handle_->watermark_ctx_.init(watermark_args);
    }
    av::FilterContext* last_ctx = &watermark_timecode_handle_->buffersrc_ctx_;
    if (add_time_code) {
      // 连接时间码过滤器
      last_ctx->link(0, watermark_timecode_handle_->timecode_ctx_, 0);
      last_ctx = &watermark_timecode_handle_->timecode_ctx_;
    }
    if (!add_watermark.empty()) {
      // 连接水印过滤器
      last_ctx->link(0, watermark_timecode_handle_->watermark_ctx_, 0);
      last_ctx = &watermark_timecode_handle_->watermark_ctx_;
    }
    last_ctx->link(0, watermark_timecode_handle_->buffersink_ctx_, 0);

    watermark_timecode_handle_->configure();
  }

  void process_filter(av::VideoFrame& in_frame) { process_subtitle_filter(in_frame); }

  void process_subtitle_filter(av::VideoFrame& in_frame) {
    if (subtitle_handle_ && subtitle_handle_->configured_) {
      // subtitles 滤镜严格依赖输入帧的 PTS/time_base。
      // 这里将其对齐到输出视频时间线（与 buffer filter 的 time_base 一致），否则字幕可能不会被渲染。
      // 再添加上时间偏移
      in_frame.setTimeBase(output_handle_.video_next_pts_.timebase());
      in_frame.setPts(output_handle_.video_next_pts_ - subtitle_handle_->time_offset_);
      subtitle_handle_->buffersrc_.writeVideoFrame(in_frame);
      av::VideoFrame filtered_frame{};
      while (subtitle_handle_->buffersink_.getVideoFrame(filtered_frame))
        process_watermark_timecode_filter(filtered_frame);
    } else
      process_watermark_timecode_filter(in_frame);
  }

  void process_watermark_timecode_filter(av::VideoFrame& in_frame) {
    if (watermark_timecode_handle_ && watermark_timecode_handle_->configured_) {
      // 时间码和水印滤镜同样依赖 PTS/time_base
      in_frame.setTimeBase(output_handle_.video_next_pts_.timebase());
      in_frame.setPts(output_handle_.video_next_pts_);
      watermark_timecode_handle_->buffersrc_.writeVideoFrame(in_frame);
      av::VideoFrame filtered_frame{};
      while (watermark_timecode_handle_->buffersink_.getVideoFrame(filtered_frame)) encode_video_frame(filtered_frame);
    } else
      encode_video_frame(in_frame);
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
    // SPDLOG_WARN("帧时间 {}", output_handle_.video_next_pts_.seconds());
    in_frame.setTimeBase(output_handle_.video_next_pts_.timebase());
    in_frame.setPts(output_handle_.video_next_pts_);
    output_handle_.video_next_pts_ += av::Timestamp{1, output_handle_.video_next_pts_.timebase()};
    if (auto out_pkt = output_handle_.video_enc_ctx_.encode(in_frame); out_pkt) {
      out_pkt.setTimeBase(output_handle_.video_next_pts_.timebase());
      out_pkt.setPts(output_handle_.video_packet_time_);
      output_handle_.video_packet_time_ +=
          av::Timestamp{out_pkt.duration() == 0 ? 1 : out_pkt.duration(), out_pkt.timeBase()};
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
      out_pkt.setPts(output_handle_.audio_packet_time_);
      output_handle_.audio_packet_time_ += av::Timestamp{
          out_pkt.duration() == 0 ? output_handle_.audio_enc_ctx_.frameSize() : out_pkt.duration(), out_pkt.timeBase()
      };
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

    const int frame_size        = output_handle_.audio_enc_ctx_.frameSize();
    DOODLE_CHICK(frame_size > 0, "ffmpeg_video: invalid audio encoder frame size");

    while (auto l_sp = audio_fifo_.read(frame_size, output_handle_.audio_enc_ctx_.sampleRate())) {
      encode_audio_frame(l_sp);
    }
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

  void start_audio() {}

  void process() {
    output_handle_.format_context_.writeHeader();
    if (intro_handle_.video_stream_.isValid()) {
      intro_handle_.process(*this, false);
    }
    if (episodes_name_handle_.video_stream_.isValid()) {
      // 这个 mp4 文件没有音频流, 会产生静音的音频流
      episodes_name_handle_.process(*this, true);
    }
    // 处理主视频流
    input_video_handle_.process(*this, false);
    // 处理音频流
    if (audio_handle_.audio_stream_.isValid()) {
      audio_handle_.process(*this, false);
    }
    if (outro_handle_.video_stream_.isValid()) {
      outro_handle_.process(*this, false);
    }
    flush_video_encoder();
    flush_subtitle_filter();

    if (output_handle_.audio_stream_.isValid()) {
      flush_audio_encoder();
    }

    output_handle_.format_context_.writeTrailer();
  }
};

ffmpeg_video::ffmpeg_video(const FSys::path& in_video_path, const FSys::path& in_out_path)
    : video_path_(in_video_path), out_path_(in_out_path) {}

ffmpeg_video::ffmpeg_video()  = default;

ffmpeg_video::~ffmpeg_video() = default;

void ffmpeg_video::process() {
  out_path_.replace_extension(".mp4");
  DOODLE_CHICK(!video_path_.empty() && FSys::exists(video_path_), "ffmpeg_video: video path is empty or not exists");
  impl l_impl{};
  l_impl.open(video_path_, out_path_);
  if (!audio_path_.empty() && FSys::exists(audio_path_)) {
    l_impl.add_audio(audio_path_);
  }
  if (!episodes_name_path_.empty() && FSys::exists(episodes_name_path_)) {
    l_impl.add_episodes_name(episodes_name_path_);
  }
  if (!intro_path_.empty() && FSys::exists(intro_path_) && !outro_path_.empty() && FSys::exists(outro_path_)) {
    l_impl.add_intro_outro(intro_path_, outro_path_);
  }
  if (!subtitle_path_.empty() && FSys::exists(subtitle_path_)) {
    l_impl.add_subtitle(subtitle_path_);
  }
  if (time_code_ || !watermark_text_.empty()) l_impl.add_time_code_watermark(time_code_, watermark_text_);

  l_impl.process();
}

void ffmpeg_video::check_video_valid(
    const FSys::path& in_video_path, const std::string& in_video_name, bool has_video_stream
) {
  DOODLE_CHICK(!in_video_path.empty(), "ffmpeg_video: {} video path is empty", in_video_name);
  DOODLE_CHICK(
      FSys::exists(in_video_path), "ffmpeg_video: {} video file not exists: {} ", in_video_name, in_video_path
  );
  DOODLE_CHICK(
      FSys::is_regular_file(in_video_path), "ffmpeg_video: {} video path is not a file: {}", in_video_name,
      in_video_path
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
    DOODLE_CHICK(l_in_video_stream.isValid(), "ffmpeg_video: {} input has no video stream", in_video_name);

    // video decoder must exist
    av::Codec l_in_video_codec = l_in_video_stream.codecParameters().decodingCodec();
    DOODLE_CHICK(!l_in_video_codec.isNull(), "ffmpeg_video: {} cannot find video decoder", in_video_name);
    DOODLE_CHICK(l_in_video_codec.canDecode(), "ffmpeg_video: {} video decoder cannot decode", in_video_name);
    // 检查帧率
    av::Rational l_in_fps = l_in_video_stream.averageFrameRate();
    DOODLE_CHICK(l_in_fps.getNumerator() == g_fps, "ffmpeg_video: {} cannot get input video fps", in_video_name);
  }

  // audio stream is optional, but if present it must be AAC + stereo (2 channels)
  if (l_in_audio_stream.isValid()) {
    av::Codec l_in_audio_codec = l_in_audio_stream.codecParameters().decodingCodec();
    DOODLE_CHICK(!l_in_audio_codec.isNull(), "ffmpeg_video: {} cannot find audio decoder", in_video_name);
    DOODLE_CHICK(l_in_audio_codec.canDecode(), "ffmpeg_video: {} audio decoder cannot decode", in_video_name);

    // DOODLE_CHICK(l_in_audio_codec.id() == AV_CODEC_ID_AAC, "ffmpeg_video: {} audio codec is not AAC", in_video_name);

    av::AudioDecoderContext l_dec_ctx{l_in_audio_stream, l_in_audio_codec};
    l_dec_ctx.open();
    DOODLE_CHICK(l_dec_ctx.channels() == 2, "ffmpeg_video: {} audio channel is not stereo", in_video_name);
  }
}

class ffmpeg_video_resize::impl {
 public:
  struct base_t {
    av::FormatContext format_context_;
    av::Stream video_stream_;
    av::Codec video_codec_;
    av::Stream audio_stream_;
    av::Codec audio_codec_;
  };
  // 过滤器处理
  struct filter_handle_t {
    av::FilterGraph graph_{};
    av::FilterContext buffersrc_ctx_{};
    av::BufferSrcFilterContext buffersrc_{};

    av::FilterContext buffersink_ctx_{};
    av::BufferSinkFilterContext buffersink_{};
    bool configured_{false};

    void init_buffersrc(const av::VideoDecoderContext& dec_ctx) {
      auto l_time_base     = dec_ctx.timeBase();
      auto l_sample_aspect = dec_ctx.sampleAspectRatio();
      DOODLE_CHICK(l_sample_aspect != av::Rational{}, "ffmpeg_video: invalid sample aspect ratio");

      const std::string l_buffer_args = std::format(
          "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}", dec_ctx.width(), dec_ctx.height(),
          static_cast<int>(dec_ctx.pixelFormat()), l_time_base.getNumerator(), l_time_base.getDenominator(),
          l_sample_aspect.getNumerator(), l_sample_aspect.getDenominator()
      );
      graph_.setAutoConvert(AVFILTER_AUTO_CONVERT_ALL);

      const av::Filter buffer_filter{"buffer"};
      const av::Filter buffersink_filter{"buffersink"};

      buffersrc_ctx_  = graph_.createFilter(buffer_filter, "in", l_buffer_args);
      buffersink_ctx_ = graph_.createFilter(buffersink_filter, "out", "");
    }

    void configure() {
      graph_.config();
      buffersrc_  = av::BufferSrcFilterContext{buffersrc_ctx_};
      buffersink_ = av::BufferSinkFilterContext{buffersink_ctx_};
      configured_ = true;
    }
  };

  struct fps_filter_handle_t : filter_handle_t {
    void init_fps_filter(const av::VideoDecoderContext& dec_ctx, int fps) {
      init_buffersrc(dec_ctx);

      const av::Filter fps_filter{"fps"};
      DOODLE_CHICK(fps_filter, "ffmpeg_video: cannot find filter 'fps'");

      const std::string fps_args = std::format("fps={}", fps);
      auto fps_ctx               = graph_.allocFilter(fps_filter, "fps");
      fps_ctx.init(fps_args);

      buffersrc_ctx_.link(0, fps_ctx, 0);
      fps_ctx.link(0, buffersink_ctx_, 0);

      configure();
    }
  };

  struct input_video : base_t {
    av::VideoDecoderContext video_dec_ctx_;

    av::AudioDecoderContext audio_dec_ctx_;
    av::ChannelLayout audio_channel_layout_;

    fps_filter_handle_t fps_filter_;

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
      if (!video_stream_.isValid()) {
        SPDLOG_WARN("ffmpeg_video: input has no video stream");
        return;
      }
      video_codec_ = video_stream_.codecParameters().decodingCodec();
      DOODLE_CHICK(!video_codec_.isNull(), "ffmpeg_video: cannot find video decoder");
      DOODLE_CHICK(video_codec_.isDecoder(), "ffmpeg_video: video decoder is not decoder");
      video_dec_ctx_ = av::VideoDecoderContext{video_stream_, video_codec_};
      video_dec_ctx_.open();

      if (video_stream_.averageFrameRate().getNumerator() != av::Rational{g_fps, 1}) {
        fps_filter_.init_fps_filter(video_dec_ctx_, g_fps);
      }
    }

    void open_audio_context() {
      for (size_t i = 0; i < format_context_.streamsCount(); ++i) {
        auto st = format_context_.stream(i);
        if (st.isAudio()) {
          audio_stream_ = st;
          break;
        }
      }
      if (!audio_stream_.isValid()) {
        SPDLOG_WARN("ffmpeg_video: input has no audio stream");
        return;
      }
      audio_codec_ = audio_stream_.codecParameters().decodingCodec();
      DOODLE_CHICK(!audio_codec_.isNull(), "ffmpeg_video: cannot find audio decoder");
      DOODLE_CHICK(audio_codec_.isDecoder(), "ffmpeg_video: audio decoder is not decoder");

      audio_dec_ctx_ = av::AudioDecoderContext{audio_stream_, audio_codec_};
      audio_dec_ctx_.open();

      audio_channel_layout_ = audio_dec_ctx_.channelLayout2().layout() == 0
                                  ? av::ChannelLayout{audio_dec_ctx_.channels()}
                                  : av::ChannelLayout{audio_dec_ctx_.channelLayout2()};
    }

    void process(ffmpeg_video_resize::impl& parent) {
      while (auto l_pkt = format_context_.readPacket()) {
        if (l_pkt.streamIndex() == video_stream_.index()) process_output_video(parent, l_pkt);
        if (l_pkt.streamIndex() == audio_stream_.index()) process_output_audio(parent, l_pkt);
      }
      flush_fps_filter(parent);
      flush_video_decoder(parent);
      flush_audio_decoder(parent);
    }

   private:
    void process_output_video(ffmpeg_video_resize::impl& in_parent, av::Packet& in_pkt) {
      auto frame = video_dec_ctx_.decode(in_pkt);
      if (!frame) return;

      if (fps_filter_.configured_) {
        fps_filter_.buffersrc_.writeVideoFrame(frame);
        av::VideoFrame filtered_frame{};
        while (fps_filter_.buffersink_.getVideoFrame(filtered_frame)) in_parent.encode_video_frame(filtered_frame);
      } else
        in_parent.encode_video_frame(frame);
    }
    void process_output_audio(ffmpeg_video_resize::impl& parent, av::Packet& in_pkt) {
      auto frame = audio_dec_ctx_.decode(in_pkt);
      if (!frame) return;

      parent.encode_audio_frame(frame);
    }

    void flush_audio_decoder(ffmpeg_video_resize::impl& parent) {
      if (!audio_dec_ctx_.isValid()) return;
      while (auto l_frame = audio_dec_ctx_.decode({})) {
        parent.encode_audio_frame(l_frame);
      }
    }
    void flush_fps_filter(ffmpeg_video_resize::impl& parent) {
      if (!fps_filter_.configured_) return;
      (void)av_buffersrc_add_frame_flags(fps_filter_.buffersrc_ctx_.raw(), nullptr, 0);
      av::VideoFrame filtered;
      while (fps_filter_.buffersink_.getVideoFrame(filtered)) parent.encode_video_frame(filtered);
    }
    void flush_video_decoder(ffmpeg_video_resize::impl& parent) {
      if (!video_dec_ctx_.isValid()) return;
      while (auto l_frame = video_dec_ctx_.decode({})) {
        parent.encode_video_frame(l_frame);
      }
    }
  };
  input_video input_video_handle_;
  // 输出视频
  struct out_video : base_t {
    av::Codec h264_codec_;
    av::VideoEncoderContext video_enc_ctx_;

    av::Timestamp video_next_pts_{};
    // 视频流包时间
    av::Timestamp video_packet_time_{};

    // 音频流
    av::AudioEncoderContext audio_enc_ctx_;
    av::Timestamp audio_next_pts_{};
    av::Timestamp audio_packet_time_{};

    // 重新调整大小组件
    av::VideoRescaler rescaler_;
    // 音频重采样组件
    av::AudioResampler resampler_;
    av::ChannelLayout audio_channel_layout_;
    bool fixed_audio_channels_{false};
    std::int32_t source_audio_sample_rate_{0};

    void open_output_video(const FSys::path& out_path, int width, int height) {
      format_context_.openOutput(out_path.string());
      open_output_video(width, height);
    }

    void open_output_video(int width, int height) {
      h264_codec_ = av::findEncodingCodec(AV_CODEC_ID_H264);
      DOODLE_CHICK(h264_codec_.isEncoder(), "ffmpeg_video: cannot find h264 encoder");

      const static av::Rational l_video_tb{1, g_fps};
      video_enc_ctx_ = av::VideoEncoderContext{h264_codec_};
      video_enc_ctx_.setWidth(width);
      video_enc_ctx_.setHeight(height);
      video_enc_ctx_.setTimeBase(l_video_tb);
      video_enc_ctx_.setPixelFormat(pick_first_supported_pix_fmt(h264_codec_, AV_PIX_FMT_YUV420P));
      // 设置码率 vbr 目标 9.9 mpbs
      constexpr static int k_bitrate = 9'900'000;
      video_enc_ctx_.setBitRate(k_bitrate);
      video_enc_ctx_.setBitRateRange({k_bitrate / 2, k_bitrate});

      video_enc_ctx_.open();

      video_stream_ = format_context_.addStream(video_enc_ctx_);
      video_stream_.setTimeBase(l_video_tb);
      video_stream_.setFrameRate(av::Rational{g_fps, 1});
      video_stream_.setAverageFrameRate(av::Rational{g_fps, 1});

      video_next_pts_    = av::Timestamp{0, l_video_tb};
      video_packet_time_ = av::Timestamp{0, l_video_tb};
    }

    void open_output_audio() {
      audio_codec_ = av::findEncodingCodec(AV_CODEC_ID_AAC);
      DOODLE_CHICK(audio_codec_.isEncoder(), "ffmpeg_video: cannot find aac encoder");
      audio_enc_ctx_ = av::AudioEncoderContext{audio_codec_};
      DOODLE_CHICK(audio_enc_ctx_.isValid(), "ffmpeg_video: cannot create aac encoder context");
      audio_enc_ctx_.setCodec(audio_codec_);
      audio_enc_ctx_.setSampleRate(48000);
      // aac 编解码器必然支持立体声
      audio_enc_ctx_.setChannelLayout(av::ChannelLayout{2});

      audio_enc_ctx_.setSampleFormat(
          pick_first_supported_sample_fmt(audio_enc_ctx_.codec(), audio_enc_ctx_.sampleFormat())
      );
      audio_enc_ctx_.setTimeBase(av::Rational{1, audio_enc_ctx_.sampleRate()});
      audio_enc_ctx_.open();

      audio_stream_ = format_context_.addStream(audio_enc_ctx_);
      audio_stream_.setTimeBase(audio_enc_ctx_.timeBase());
      audio_next_pts_       = av::Timestamp{0, audio_stream_.timeBase()};
      audio_packet_time_    = av::Timestamp{0, audio_stream_.timeBase()};
      audio_channel_layout_ = audio_enc_ctx_.channelLayout2().layout() == 0
                                  ? av::ChannelLayout{audio_enc_ctx_.channels()}
                                  : av::ChannelLayout{audio_enc_ctx_.channelLayout2()};
    }
    void add_rescaler(
        int in_width, int in_height, AVPixelFormat in_pix_fmt, std::int32_t in_src_width, std::int32_t in_src_height,
        AVPixelFormat in_src_pix_fmt
    ) {
      rescaler_ = av::VideoRescaler{in_width, in_height, in_pix_fmt, in_src_width, in_src_height, in_src_pix_fmt};
    }
    void add_resampler(const av::AudioDecoderContext& in_audio_enc_ctx, av::ChannelLayout in_audio_channel_layout) {
      if (audio_enc_ctx_.sampleRate() == in_audio_enc_ctx.sampleRate() &&
          audio_channel_layout_.layout() == in_audio_channel_layout.layout() &&
          audio_enc_ctx_.sampleFormat().get() == in_audio_enc_ctx.sampleFormat().get())
        return;
      fixed_audio_channels_     = audio_channel_layout_.layout() == in_audio_channel_layout.layout();
      source_audio_sample_rate_ = in_audio_enc_ctx.sampleRate();
      resampler_.init(
          audio_channel_layout_.layout(), audio_enc_ctx_.sampleRate(), audio_enc_ctx_.sampleFormat().get(),
          in_audio_channel_layout.layout(), in_audio_enc_ctx.sampleRate(), in_audio_enc_ctx.sampleFormat().get()
      );
    }

    void flush() {
      // flush audio resampler
      if (resampler_.isValid()) {
        const auto l_frame_size = audio_enc_ctx_.frameSize();
        DOODLE_CHICK(l_frame_size > 0, "ffmpeg_video: invalid audio encoder frame size");
        av::AudioSamples l_resampled_frame{};
        l_resampled_frame.init(
            audio_enc_ctx_.sampleFormat(), l_frame_size, audio_channel_layout_.layout(), audio_enc_ctx_.sampleRate()
        );
        while (resampler_.pop(l_resampled_frame, true)) {
          encode_audio_frame(l_resampled_frame);
          l_resampled_frame = av::AudioSamples{};
          l_resampled_frame.init(
              audio_enc_ctx_.sampleFormat(), l_frame_size, audio_channel_layout_.layout(), audio_enc_ctx_.sampleRate()
          );
        }
      }

      // Flush video encoder
      const int l_out_video_index = video_stream_.index();
      while (auto out_pkt = video_enc_ctx_.encode()) {
        out_pkt.setTimeBase(video_next_pts_.timebase());
        out_pkt.setPts(video_packet_time_);
        video_packet_time_ += av::Timestamp{out_pkt.duration() == 0 ? 1 : out_pkt.duration(), out_pkt.timeBase()};
        out_pkt.setStreamIndex(l_out_video_index);
        format_context_.writePacket(out_pkt);
      }
      const int l_out_audio_index = audio_stream_.index();
      // Flush audio encoder
      while (auto out_pkt = audio_enc_ctx_.encode()) {
        out_pkt.setTimeBase(audio_next_pts_.timebase());
        out_pkt.setPts(audio_packet_time_);
        audio_packet_time_ += av::Timestamp{
            out_pkt.duration() == 0 ? audio_enc_ctx_.frameSize() : out_pkt.duration(), out_pkt.timeBase()
        };
        out_pkt.setStreamIndex(l_out_audio_index);
        format_context_.writePacket(out_pkt);
      }
    }

    void resample_audio_frame(av::AudioSamples& in_frame) {
      auto l_time_base = in_frame.timeBase();
      auto l_time_pts  = in_frame.pts();
      if (!fixed_audio_channels_) {
        av::frame::set_channel_layout(in_frame.raw(), audio_channel_layout_.layout());
      }
      if (in_frame.sampleRate() <= 0) av::frame::set_sample_rate(in_frame.raw(), source_audio_sample_rate_);

      if (resampler_.isValid()) {
        // in_frame.setTimeBase(l_time_base);
        // in_frame.setPts(l_time_pts);
        resampler_.push(in_frame);
        while (auto resampled_frame = resampler_.pop(audio_enc_ctx_.frameSize())) encode_audio_frame(resampled_frame);
      } else
        encode_audio_frame(in_frame);
    }

    void encode_audio_frame(av::AudioSamples& in_frame) {
      in_frame.setTimeBase(audio_next_pts_.timebase());
      in_frame.setPts(audio_next_pts_);
      audio_next_pts_ += av::Timestamp{in_frame.samplesCount(), in_frame.timeBase()};

      if (auto out_pkt = audio_enc_ctx_.encode(in_frame); out_pkt) {
        out_pkt.setTimeBase(audio_stream_.timeBase());
        out_pkt.setPts(audio_packet_time_);
        audio_packet_time_ += av::Timestamp{
            out_pkt.duration() == 0 ? audio_enc_ctx_.frameSize() : out_pkt.duration(), out_pkt.timeBase()
        };
        out_pkt.setStreamIndex(audio_stream_.index());
        format_context_.writePacket(out_pkt);
      }
    }
    void encode_video_frame(av::VideoFrame& in_frame) {
      // SPDLOG_WARN("帧时间 {}", video_next_pts_.seconds());
      in_frame.setTimeBase(video_next_pts_.timebase());
      in_frame.setPts(video_next_pts_);
      video_next_pts_ += av::Timestamp{1, video_next_pts_.timebase()};
      if (auto out_pkt = video_enc_ctx_.encode(in_frame); out_pkt) {
        out_pkt.setTimeBase(video_next_pts_.timebase());
        out_pkt.setPts(video_packet_time_);
        video_packet_time_ += av::Timestamp{out_pkt.duration() == 0 ? 1 : out_pkt.duration(), out_pkt.timeBase()};
        out_pkt.setStreamIndex(video_stream_.index());
        format_context_.writePacket(out_pkt);
      }
    }
  };
  out_video output_handle_;
  out_video output_low_handle_;

  void encode_audio_frame(av::AudioSamples& in_frame) {
    output_handle_.resample_audio_frame(in_frame);
    output_low_handle_.resample_audio_frame(in_frame);
  }
  void encode_video_frame(av::VideoFrame& in_frame) {
    auto l_f = output_handle_.rescaler_.isValid() ? output_handle_.rescaler_.rescale(in_frame) : in_frame;
    output_handle_.encode_video_frame(l_f);
    l_f = output_low_handle_.rescaler_.isValid() ? output_low_handle_.rescaler_.rescale(in_frame) : in_frame;
    output_low_handle_.encode_video_frame(l_f);
  }

 public:
  impl()  = default;
  ~impl() = default;

  void open(const FSys::path& in_path) {
    input_video_handle_.open_format_context(in_path);
    input_video_handle_.open_video_context();
    input_video_handle_.open_audio_context();
  }
  void open_out(
      const FSys::path& in_high_path, const cv::Size& in_high_size, const FSys::path& in_low_path,
      const cv::Size& in_low_size
  ) {
    const int l_src_width  = input_video_handle_.video_dec_ctx_.width();
    const int l_src_height = input_video_handle_.video_dec_ctx_.height();
    const auto l_pix_fmt   = input_video_handle_.video_dec_ctx_.pixelFormat().get();
    output_handle_.open_output_video(in_high_path.string(), in_high_size.width, in_high_size.height);
    if (in_high_size.width != l_src_width || in_high_size.height != l_src_height)
      output_handle_.add_rescaler(
          in_high_size.width, in_high_size.height, l_pix_fmt, l_src_width, l_src_height, l_pix_fmt
      );
    output_handle_.open_output_audio();
    output_handle_.add_resampler(input_video_handle_.audio_dec_ctx_, input_video_handle_.audio_channel_layout_);

    // 低码率输出
    output_low_handle_.open_output_video(in_low_path.string(), in_low_size.width, in_low_size.height);
    if (in_low_size.width != l_src_width || in_low_size.height != l_src_height)
      output_low_handle_.add_rescaler(
          in_low_size.width, in_low_size.height, l_pix_fmt, l_src_width, l_src_height, l_pix_fmt
      );
    output_low_handle_.open_output_audio();
    output_low_handle_.add_resampler(input_video_handle_.audio_dec_ctx_, input_video_handle_.audio_channel_layout_);
  }

  void process() {
    output_handle_.format_context_.writeHeader();
    output_low_handle_.format_context_.writeHeader();

    input_video_handle_.process(*this);
    output_handle_.flush();
    output_low_handle_.flush();

    output_handle_.format_context_.writeTrailer();
    output_low_handle_.format_context_.writeTrailer();
  }
};
void ffmpeg_video_resize::process() {
  impl l_impl{};
  auto l_now = chrono::system_clock::now();
  l_impl.open(video_path_);
  l_impl.open_out(out_high_path_, high_size_, out_low_path_, low_size_);
  l_impl.process();
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_long_task(), "ffmpeg_video_resize: {} resize video used {:%H:%M:%S} seconds", video_path_,
      chrono::system_clock::now() - l_now
  );
}

}  // namespace doodle