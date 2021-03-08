#pragma once
#include <lib/MotionGlobal.h>
struct AVFormatContext;
struct AVOutputFormat;
struct AVStream;
struct AVCodec;
struct AVCodecContext;

namespace doodle::motion::kernel {

class MayaVideo {
 private:
  FSys::path p_file;

  int error;
  AVFormatContext* p_avformat_context;
  AVOutputFormat* p_output_format;
  AVStream* p_stream;

  AVCodec* p_codec;
  AVCodecContext* p_codec_context;

  void check(bool verb, const std::string& err);

 public:
  MayaVideo(FSys::path path);
  ~MayaVideo();
};

}  // namespace doodle::motion::kernel