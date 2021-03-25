#include <corelib/FileWarp/VideoSequence.h>
#include <corelib/Exception/Exception.h>

#include <corelib/core/coreset.h>

#include <opencv2/opencv.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/format.hpp>

namespace doodle {
VideoSequence::VideoSequence(decltype(p_paths) paths)
    : LongTerm(),
      p_paths(std::move(paths)) {
  for (auto&& path : p_paths) {
    auto ex = path.extension();
    if (ex != ".mp4" ||
        ex != ".avi")
      throw DoodleError("不是MP4文件, 暂时不支持");
  }
}

void VideoSequence::connectVideo(const FSys::path& out_path) {
  //验证输出路径
  auto k_out_path = p_paths[0].parent_path() /
                    boost::uuids::to_string(coreSet::getSet().getUUID()).append(".mp4");

  if (!out_path.empty())
    k_out_path = out_path;

  //验证输出文件
  if (!FSys::exists(k_out_path.parent_path()))
    FSys::create_directories(k_out_path.parent_path());

  auto k_voide_input   = cv::VideoCapture{};
  auto k_voide_out     = cv::VideoWriter{k_out_path.generic_string(),
                                     cv::VideoWriter::fourcc('D', 'I', 'V', 'X'),
                                     25,
                                     cv::Size(1280, 720)};
  auto k_image         = cv::Mat{};
  auto k_image_resized = cv::Mat{};
  const static cv::Size k_size{1280, 720};
  const auto k_len = boost::numeric_cast<float>(p_paths.size());
  auto k_i         = float{0};
  for (auto path : p_paths) {
    if (k_voide_input.open(path.generic_string())) {
      //获得总帧数
      auto k_frame_count = k_voide_input.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT);

      while (k_voide_input.read(k_image)) {
        auto k_frame = k_voide_input.get(cv::VideoCaptureProperties::CAP_PROP_POS_FRAMES);
        if (k_image.cols != 1280 || k_image.rows != 720)
          cv::resize(k_image, k_image_resized, k_size);
        else
          k_image_resized = k_image;

        k_voide_out << k_image_resized;
        this->progress(
            boost::numeric_cast<int>(
                (
                    (k_frame / (k_frame_count * k_len) +
                     (k_i / k_len))  //
                    )                //
                * 100)               //
        );
      }
    } else {
      throw DoodleError("不支持的格式");
    }
    ++k_i;
  }
  boost::format message{"完成视频 %s"};
  message % k_out_path.generic_string();
  this->messagResult(message.str());
  this->finished();
}
}  // namespace doodle