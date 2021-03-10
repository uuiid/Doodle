#include <lib/kernel/Maya/MayaVideo.h>

#include <lib/kernel/doodleFFmpeg.h>
#include <lib/kernel/Exception.h>
#include <lib/kernel/Maya/MayaRenderOpenGL.h>
#include <lib/kernel/ExeWarp/FFmpegWarp.h>

#include <iostream>
#include <sstream>

#include <Maya/MGlobal.h>
#include <Maya/MAnimControl.h>
#include <Maya/MFileIO.h>

namespace doodle::motion::kernel {
MayaVideo::MayaVideo(FSys::path file)
    : p_file(std::move(file)),
      p_file_image(),
      p_view(std::make_unique<MayaRenderOpenGL>(1280, 720)),
      p_ffmpeg(std::make_unique<FFmpegWarp>()) {
}

void MayaVideo::save() {
  auto p_path = FSys::temp_directory_path() / "doodle";
  // auto p_path = FSys::path{"D:/tmp"} / "doodle";
  if (!FSys::exists(p_path))
    FSys::create_directories(p_path);

  p_view->getFileName.connect([=](const MTime& time) -> MString {
    MString fileName{};
    MString k_tmp{};
    k_tmp.setUTF8(p_path.generic_u8string().c_str());
    fileName += k_tmp;
    fileName += "/";

    //添加uuid名称
    k_tmp.setUTF8(this->p_file.stem().generic_u8string().c_str());
    fileName += k_tmp;
    fileName += ".";

    //添加序列号
    std::stringstream str{};
    str << std::setw(5) << std::setfill('0') << (time.value());
    fileName += str.str().c_str();

    fileName += ".png";
    this->p_file_image.emplace_back(std::make_shared<FSys::path>(fileName.asWChar()));
    return fileName;
  });
  p_view->save(MAnimControl::animationStartTime(), MAnimControl::animationEndTime());

  for (auto&& it : p_file_image)
    if (!FSys::exists(*it))
      throw MayaError("not create file " + it->generic_u8string());

  p_ffmpeg->imageToVideo(p_file_image, p_file, "doodle");
  if (FSys::exists(p_file)) throw NotFileError("nor create file" + p_file.generic_u8string());
}

MayaVideo::~MayaVideo() {
  for (auto&& it : p_file_image) {
    FSys::remove(*it);
  }
}

// void MayaVideo::check(bool verb, const std::string& err) {
//   if (verb) {
//     if (error != 0) {
//       std::string str{"0", 1024};
//       av_strerror(error, str.data(), str.size());
//       avformat_free_context(p_avformat_context);
//       throw FFmpegError(str);
//     } else {
//       throw FFmpegError(err);
//     }
//   }
// }

// MayaVideo::MayaVideo(FSys::path path, int width, int height)
//     : p_file(std::move(path)),
//       error(0),
//       p_avformat_context(),
//       p_output_format(),
//       p_stream(),
//       p_codec(),
//       p_codec_context(),
//       p_frame_rgb(),
//       p_frame_yuv(),
//       p_sws_context(),
//       p_packet(av_packet_alloc()),
//       p_width(std::move(width)),
//       p_height(std::move(height)),
//       p_iframe(0) {
//   error = avformat_alloc_output_context2(&p_avformat_context, nullptr, nullptr, p_file.generic_u8string().c_str());
//   check(error < 0, "");

//   p_sws_context = sws_getContext(
//       p_width, p_height,
//       AV_PIX_FMT_RGB24, p_width, p_height, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR,
//       nullptr, nullptr, nullptr);

//   p_output_format = p_avformat_context->oformat;
//   if (p_output_format->video_codec != AV_CODEC_ID_NONE) {
//     p_codec = avcodec_find_encoder(p_output_format->video_codec);
//     this->check(p_codec == nullptr, "无法创建编码器");

//     p_stream = avformat_new_stream(p_avformat_context, this->p_codec);
//     this->check(p_stream == nullptr, "无法创建av流");
//     p_stream->id = p_avformat_context->nb_streams - 1;

//     p_codec_context = avcodec_alloc_context3(p_codec);
//     this->check(p_codec_context == nullptr, "无法创建流上下文");

//     p_codec_context->width     = p_width;
//     p_codec_context->height    = p_height;
//     p_codec_context->pix_fmt   = AV_PIX_FMT_YUV420P;
//     p_codec_context->time_base = av_make_q(1, 25);

//     if (p_output_format->flags & AVFMT_GLOBALHEADER) {
//       p_output_format->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
//     }

//     AVDictionary* k_av_dir = nullptr;
//     av_dict_set(&k_av_dir, "preset", "slow", 0);
//     av_dict_set(&k_av_dir, "crf", "20", 0);

//     avcodec_open2(p_codec_context, p_codec, &k_av_dir);
//     av_dict_free(&k_av_dir);

//     p_stream->time_base = av_make_q(1, 25);

//     av_dump_format(p_avformat_context, 0, p_file.generic_u8string().c_str(), 1);
//     avio_open(&p_avformat_context->pb, p_file.generic_u8string().c_str(), AVIO_FLAG_WRITE);

//     error = avformat_write_header(p_avformat_context, &k_av_dir);
//     this->check(error == 0, "");
//     av_dict_free(&k_av_dir);

//     p_frame_rgb = av_frame_alloc();
//     this->check(p_frame_rgb == nullptr, "指针为空");
//     p_frame_rgb->format = AV_PIX_FMT_RGB24;
//     p_frame_rgb->width  = p_width;
//     p_frame_rgb->height = p_height;

//     error = av_frame_get_buffer(p_frame_rgb, 1);
//     this->check(error != 0, "");

//     p_frame_yuv         = av_frame_alloc();
//     p_frame_yuv->format = AV_PIX_FMT_YUV420P;
//     p_frame_yuv->width  = p_width;
//     p_frame_yuv->height = p_height;
//     error               = av_frame_get_buffer(p_frame_yuv, 1);
//     this->check(error != 0, "");
//   }
// }

// void MayaVideo::addFrame(const void* data_, int rgba) {
//   auto k_data = (uint8_t*)data_;
//   for (int32_t x = 0; x < p_height; ++x) {
//     for (int32_t y = 0; y < p_width; ++y) {
//       p_frame_rgb->data[0][y * p_width + 3 * x + 0] = k_data[y * 4 * p_width + 4 * x + 2];
//       p_frame_rgb->data[0][y * p_width + 3 * x + 1] = k_data[y * 4 * p_width + 4 * x + 1];
//       p_frame_rgb->data[0][y * p_width + 3 * x + 2] = k_data[y * 4 * p_width + 4 * x + 0];
//     }
//   }
//   sws_scale(p_sws_context, p_frame_rgb->data, p_frame_rgb->linesize, 0,
//             p_height, p_frame_yuv->data, p_frame_yuv->linesize);
//   av_init_packet(p_packet);
//   // p_packet->data = nullptr;
//   // p_packet->size = 0;

//   p_frame_yuv->pts = p_iframe;
//   int got_output{0};

//   error = avcodec_send_frame(p_codec_context, p_frame_yuv);
//   while (error >= 0) {
//     error = avcodec_receive_packet(p_codec_context, p_packet);
//     this->check(error != 0, "");
//     av_packet_rescale_ts(p_packet, p_codec_context->time_base, p_stream->time_base);
//     p_packet->stream_index = p_stream->index;

//     error = av_interleaved_write_frame(p_avformat_context, p_packet);
//     this->check(error != 0, "");

//     std::cout << "writing frame " << p_iframe++ << p_packet->size << std::endl;
//     av_packet_unref(p_packet);
//     // error = fwrite(p_packet->data, 1, p_packet->size);
//   }
//   // if (got_output) {
//   //   av_interleaved_write_frame(p_avformat_context, p_packet);
//   //   av_packet_unref(p_packet);
//   // }
// }

// MayaVideo::~MayaVideo() {
//   av_write_trailer(p_avformat_context);
//   if (p_avformat_context->flags & AVFMT_NOFILE)
//     avio_close(p_avformat_context->pb);

//   sws_freeContext(p_sws_context);
//   av_frame_free(&p_frame_yuv);
//   av_frame_free(&p_frame_rgb);
//   avformat_free_context(p_avformat_context);
// }
}  // namespace doodle::motion::kernel