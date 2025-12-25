#include "ffmpeg_video.h"

#include "doodle_core/exception/exception.h"

#include <avcpp/codeccontext.h>
#include <avcpp/formatcontext.h>
#include <avcpp/stream.h>
#include <optional>
#include <system_error>

extern "C" {
#include <libavutil/error.h>
}

namespace {

auto is_eof_error(const std::error_code& ec) -> bool {
  return ec.category() == av::ffmpeg_category() && ec.value() == AVERROR_EOF;
}

auto pick_first_video_stream_index(av::FormatContext& ctx) -> av::Stream {
  for (size_t i = 0; i < ctx.streamsCount(); ++i) {
    auto st = ctx.stream(i);
    if (st.isVideo()) {
      return st;
    }
  }
  return av::Stream{};
}

auto pick_first_audio_stream_index(av::FormatContext& ctx) -> av::Stream {
  for (size_t i = 0; i < ctx.streamsCount(); ++i) {
    auto st = ctx.stream(i);
    if (st.isAudio()) {
      return st;
    }
  }
  return av::Stream{};
}

auto read_next_packet_for_stream(av::FormatContext& ctx, int desired_stream_index) -> std::optional<av::Packet> {
  std::error_code ec{};
  av::OptionalErrorCode oec{ec};

  while (true) {
    auto pkt = ctx.readPacket(oec);
    if (ec) {
      if (is_eof_error(ec)) {
        return std::nullopt;
      }
      throw std::system_error(ec);
    }
    if (!pkt || pkt.isNull()) {
      continue;
    }
    if (pkt.streamIndex() != desired_stream_index) {
      continue;
    }
    return pkt;
  }
}

auto packet_ts(const av::Packet& pkt) -> av::Timestamp {
  // Prefer DTS for muxing order; fallback to PTS.
  auto dts = pkt.dts();
  if (dts.isValid() && !dts.isNoPts()) {
    return dts;
  }
  return pkt.pts();
}

auto rewrite_packet_timestamps(av::Packet& pkt, const av::Rational& out_tb) -> void {
  auto pts = pkt.pts();
  if (pts.isValid() && !pts.isNoPts()) {
    pkt.setPts(av::Timestamp{pts.timestamp(out_tb), out_tb});
  }
  auto dts = pkt.dts();
  if (dts.isValid() && !dts.isNoPts()) {
    pkt.setDts(av::Timestamp{dts.timestamp(out_tb), out_tb});
  }
  pkt.setTimeBase(out_tb);
}

}  // namespace

namespace doodle {
void ffmpeg_video::process() {
  //   ffmpeg处理视频

  // 打开视频文件
  av::FormatContext l_format_context{};
  l_format_context.openInput(video_path_.string());
  l_format_context.findStreamInfo();

  av::Stream l_in_video_stream = pick_first_video_stream_index(l_format_context);
  DOODLE_CHICK(l_in_video_stream.isValid(), "ffmpeg_video: input has no video stream");

  // 打开输出文件
  av::FormatContext l_output_format_context{};
  l_output_format_context.openOutput(out_path_.string());

  // 仅拷贝输入视频的第一个视频流
  auto l_out_video_stream = l_output_format_context.addStream();
  l_out_video_stream.setCodecParameters(l_in_video_stream.codecParameters());
  l_out_video_stream.setTimeBase(l_in_video_stream.timeBase());
  l_out_video_stream.setFrameRate(l_in_video_stream.frameRate());
  l_out_video_stream.setAverageFrameRate(l_in_video_stream.averageFrameRate());
  l_out_video_stream.setSampleAspectRatio(l_in_video_stream.sampleAspectRatio());
  const int l_out_video_index = l_out_video_stream.index();

  // 添加音频流
  std::optional<av::FormatContext> l_audio_format_context{};
  av::Stream l_audio_stream{};
  int l_out_audio_index = -1;

  if (!audio_path_.empty()) {
    l_audio_format_context.emplace();
    l_audio_format_context->openInput(audio_path_.string());
    l_audio_format_context->findStreamInfo();

    l_audio_stream = pick_first_audio_stream_index(*l_audio_format_context);
    DOODLE_CHICK(l_audio_stream.isValid(), "ffmpeg_video: audio input has no audio stream");

    auto l_in_audio_stream  = l_audio_format_context->stream(static_cast<size_t>(l_audio_stream.index()));
    auto l_out_audio_stream = l_output_format_context.addStream();
    l_out_audio_stream.setCodecParameters(l_in_audio_stream.codecParameters());
    l_out_audio_stream.setTimeBase(l_in_audio_stream.timeBase());
    const int l_tmp_out_audio_index = l_out_audio_stream.index();
    l_out_audio_index               = l_tmp_out_audio_index;
  }

  l_output_format_context.writeHeader();

  std::optional<av::Packet> l_next_video = read_next_packet_for_stream(l_format_context, l_in_video_stream.index());
  std::optional<av::Packet> l_next_audio{};
  if (l_audio_format_context && l_audio_stream.isValid()) {
    l_next_audio = read_next_packet_for_stream(*l_audio_format_context, l_audio_stream.index());
  }

  while (l_next_video || l_next_audio) {
    const bool take_video = [&]() {
      if (l_next_video && l_next_audio) {
        return packet_ts(*l_next_video) <= packet_ts(*l_next_audio);
      }
      return static_cast<bool>(l_next_video);
    }();

    if (take_video) {
      auto pkt    = std::move(*l_next_video);
      auto out_st = l_output_format_context.stream(static_cast<size_t>(l_out_video_index));
      rewrite_packet_timestamps(pkt, out_st.timeBase());
      pkt.setStreamIndex(l_out_video_index);
      l_output_format_context.writePacket(pkt);
      l_next_video = read_next_packet_for_stream(l_format_context, l_in_video_stream.index());
    } else {
      auto pkt    = std::move(*l_next_audio);
      auto out_st = l_output_format_context.stream(static_cast<size_t>(l_out_audio_index));
      rewrite_packet_timestamps(pkt, out_st.timeBase());
      pkt.setStreamIndex(l_out_audio_index);
      l_output_format_context.writePacket(pkt);
      l_next_audio = read_next_packet_for_stream(*l_audio_format_context, l_audio_stream.index());
    }
  }

  l_output_format_context.writeTrailer();
}

}  // namespace doodle