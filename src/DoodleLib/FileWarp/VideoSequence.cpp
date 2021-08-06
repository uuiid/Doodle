#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/FileWarp/VideoSequence.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/core/DoodleLib.h>
#include <DoodleLib/threadPool/ThreadPool.h>

#include <opencv2/opencv.hpp>

namespace doodle {
VideoSequence::VideoSequence(decltype(p_paths) paths)
    : p_paths(std::move(paths)) {
  for (auto&& path : p_paths) {
    auto ex = path.extension();
    if (ex != ".mp4" && ex != ".avi")
      throw DoodleError("不是可以处理的视频文件, 暂时不支持");
  }
}

void VideoSequence::connectVideo(const FSys::path& out_path) {
  //验证输出路径
  auto k_out_path = p_paths[0].parent_path() /
                    boost::uuids::to_string(CoreSet::getSet().getUUID()).append(".mp4");

  if (!out_path.empty())
    k_out_path = out_path;

  //验证输出文件
  if (!FSys::exists(k_out_path.parent_path()))
    FSys::create_directories(k_out_path.parent_path());

  auto k_video_input   = cv::VideoCapture{};
  auto k_video_out     = cv::VideoWriter{k_out_path.generic_string(),
                                     cv::VideoWriter::fourcc('D', 'I', 'V', 'X'),
                                     25,
                                     cv::Size(1280, 720)};
  auto k_image         = cv::Mat{};
  auto k_image_resized = cv::Mat{};
  const static cv::Size k_size{1280, 720};
  const auto k_len = boost::numeric_cast<float>(p_paths.size());
  // 这里开始排序
  std::sort(p_paths.begin(), p_paths.end(),
            [](const FSys::path& k_l, const FSys::path& k_r) { return k_l.stem() < k_r.stem(); });

  for (const auto& path : p_paths) {
    if (k_video_input.open(path.generic_string())) {
      //获得总帧数
      auto k_frame_count = k_video_input.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT);

      while (k_video_input.read(k_image)) {
        auto k_frame = k_video_input.get(cv::VideoCaptureProperties::CAP_PROP_POS_FRAMES);
        if (k_image.cols != 1280 || k_image.rows != 720)
          cv::resize(k_image, k_image_resized, k_size);
        else
          k_image_resized = k_image;

        k_video_out << k_image_resized;
        p_term->sig_progress(
            boost::numeric_cast<int>(
                (
                    ((k_frame / k_frame_count) / k_len)  //
                    )                                    //
                )                                        //
        );
      }
    } else {
      throw DoodleError("不支持的格式");
    }
  }

  p_term->sig_message_result(fmt::format("完成视频 {}", k_out_path));
  p_term->sig_finished();
}

long_term_ptr VideoSequence::connectVideo_asyn(const FSys::path& path) {
  auto k_ptr = shared_from_this();
  DoodleLib::Get().get_thread_pool()->enqueue(
      [k_ptr, path]() { k_ptr->connectVideo(path); });
  return p_term;
}
}  // namespace doodle
