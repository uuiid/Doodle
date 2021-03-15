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

}  // namespace doodle::motion::kernel