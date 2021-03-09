#pragma once
#include <lib/MotionGlobal.h>
#include <lib/kernel/Maya/MayaRenderOpenGL.h>
// struct AVFormatContext;
// struct AVOutputFormat;
// struct AVStream;
// struct AVCodec;
// struct AVCodecContext;
// struct AVFrame;
// struct SwsContext;
// struct AVPacket;
namespace doodle::motion::kernel {

class MayaVideo {
 private:
  FSys::path p_file;

  std::vector<std::shared_ptr<FSys::path>> p_file_image;
  std::unique_ptr<MayaRenderOpenGL> p_view;
  std::unique_ptr<FFmpegWarp> p_ffmpeg;

 public:
  MayaVideo(FSys::path file);
  ~MayaVideo();
  void save();
};

// class MayaVideo {
//  private:
//   FSys::path p_file;

//   int error;
//   AVFormatContext* p_avformat_context;
//   AVOutputFormat* p_output_format;
//   AVStream* p_stream;

//   AVCodec* p_codec;
//   AVCodecContext* p_codec_context;
//   AVFrame* p_frame_rgb;
//   AVFrame* p_frame_yuv;

//   SwsContext* p_sws_context;
//   AVPacket* p_packet;
//   int p_width;
//   int p_height;
//   int32_t p_iframe;
//   void check(bool verb, const std::string& err);

//  public:
//   MayaVideo(FSys::path path, int width, int height);

//   void addFrame(const void* data, int rgba);

//   ~MayaVideo();
// };

}  // namespace doodle::motion::kernel