#include <gtest/gtest.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavresample/avresample.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
#include <filesystem>
#include <fstream>
TEST(ffmpeg, imageSequeTovideo) {
  // av_register_all();
  AVFormatContext* oformat_context = nullptr;
  const char* out_filename         = "D:/test.mp4";
  int error{};

  error = avformat_alloc_output_context2(&oformat_context, nullptr, nullptr, out_filename);
  ASSERT_TRUE(error >= 0);
  ASSERT_TRUE(oformat_context);

  AVCodec* codec = nullptr;

  // {
  //   void* i = nullptr;
  //   const AVCodec* p;
  //   while ((p = av_codec_iterate(&i))) {
  //     std::cout << p->name << std::endl;
  //   }
  // }

  // codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
  codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
  ASSERT_TRUE(codec);
  oformat_context->streams[0] = avformat_new_stream(oformat_context, codec);
  ASSERT_TRUE(oformat_context->streams[0]);

  AVCodecContext* codec_context = nullptr;
  codec_context                 = avcodec_alloc_context3(codec);
  if (oformat_context->oformat->flags & AVFMT_GLOBALHEADER)
    // stream->codecpar->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  codec_context->time_base = av_make_q(1, 25);
  codec_context->framerate = av_make_q(25, 1);
  codec_context->pix_fmt   = AV_PIX_FMT_YUV420P;
  codec_context->width     = 1280;
  codec_context->height    = 720;
  codec_context->bit_rate  = 400000;
  codec_context->codec_tag = 0;

  // stream->codecpar->bit_rate = 400000;
  // stream->codecpar->width    = 1280;
  // stream->codecpar->height   = 720;
  // stream->codecpar->time_base = av_make_q(1, 25);

  // stream->codecpar->pix_fmt   = AV_PIX_FMT_YUV420P;
  // stream->codecpar->codec_tag = 0;
  error = avcodec_parameters_from_context(oformat_context->streams[0]->codecpar, codec_context);
  ASSERT_TRUE(error >= 0);

  av_dump_format(oformat_context, 0, out_filename, 1);

  if (!(oformat_context->oformat->flags & AVFMT_NOFILE)) {
    error = avio_open(&oformat_context->pb, out_filename, AVIO_FLAG_WRITE);
    ASSERT_TRUE(error >= 0);
  }

  error = avformat_write_header(oformat_context, NULL);
  // std::string str{(size_t)60, '0'};
  // str.resize(2048);
  // av_make_error_string(str.data(), 2048, error);
  // std::cout << str << std::endl;
  ASSERT_TRUE(error >= 0);

  int index             = 0;
  unsigned char* mydata = new unsigned char[2048 * 2048];
  AVPacket* packet      = av_packet_alloc();
  ASSERT_TRUE(packet);

  av_init_packet(packet);
  packet->flags |= AV_PKT_FLAG_KEY;

  packet->stream_index = oformat_context->streams[0]->index;
  const std::filesystem::path path{R"(D:\tmp\tmp)"};
  std::fstream file{};
  int i = 0;
  for (auto&& it_p : std::filesystem::directory_iterator(path)) {
    if (it_p.is_regular_file()) {
      file.open(it_p, std::ios::binary | std::ios::in);

      if (!file.good()) std::runtime_error("");
      // FILE* file_;
      // fopen_s(&file_, it_p.path().generic_u8string().c_str(), "rb");
      // packet->size = fread(mydata, 1, 2048 * 2048, file_);
      packet->size = (int)it_p.file_size();
      file.read((char*)mydata, packet->size);
      packet->data = mydata;
      packet->pts  = i;
      // packet->dts  = 1;

      file.close();
      // fclose(file_);

      error = av_interleaved_write_frame(oformat_context, packet);
      ASSERT_TRUE(error >= 0);
      av_packet_unref(packet);
      ++i;
      std::cout << "write " << it_p /* << " \n\n " << mydata */ << "\n\nimage to stream" << std::endl;
    }
  }
  av_packet_free(&packet);
  av_write_trailer(oformat_context);
  delete[] mydata;
  if (oformat_context && !(oformat_context->oformat->flags & AVFMT_NOFILE))
    avio_close(oformat_context->pb);
  avformat_free_context(oformat_context);
}