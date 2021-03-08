#include <lib/kernel/Maya/MayaVideo.h>

#include <lib/kernel/doodleFFmpeg.h>
#include <lib/kernel/Exception.h>

namespace doodle::motion::kernel {
void MayaVideo::check(bool verb, const std::string& err) {
  if (verb) {
    if (error != 0) {
      std::string str{"0", 1024};
      av_strerror(error, str.data(), str.size());
      avformat_free_context(p_avformat_context);
      throw FFmpegError(str);
    } else {
      throw FFmpegError(err);
    }
  }
}

MayaVideo::MayaVideo(FSys::path path)
    : p_file(std::move(path)),
      error(0),
      p_avformat_context(),
      p_output_format(),
      p_stream(),
      p_codec(),
      p_codec_context() {
  error = avformat_alloc_output_context2(&p_avformat_context, nullptr, nullptr, p_file.generic_u8string().c_str());
  check(error < 0);

  p_output_format = p_avformat_context->oformat;
  if (p_output_format->video_codec != AV_CODEC_ID_NONE) {
    p_code = avcodec_find_encoder(p_output_format->video_codec);
    this->check(p_code == nullptr, "无法创建编码器");

    p_stream = avformat_new_stream(p_avformat_context, this->p_codec);
    this->check(p_stream == nullptr, "无法创建av流");
    p_stream->id = p_avformat_context->nb_streams - 1;

    p_codec_context = avcodec_alloc_context3(p_codec);
    this->check(p_codec_context == nullptr, "无法创建流上下文");

    // p_codec_context->width =
  }
}

MayaVideo::~MayaVideo() {
}
}  // namespace doodle::motion::kernel