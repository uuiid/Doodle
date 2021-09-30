#include <doodle_lib/Exception/exception.h>
#include <doodle_lib/file_warp/video_sequence.h>
#include <doodle_lib/Metadata/episodes.h>
#include <doodle_lib/Metadata/shot.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/lib_warp/std_warp.h>
#include <doodle_lib/thread_pool/thread_pool.h>

#include <opencv2/opencv.hpp>
namespace doodle {
video_sequence::video_sequence(std::vector<FSys::path> paths)
    : p_paths(std::move(paths)),
      p_name() {
  for (auto&& path : p_paths) {
    auto ex = path.extension();
    if (ex != ".mp4" && ex != ".avi")
      throw doodle_error("不是可以处理的视频文件, 暂时不支持");
  }
  // 这里开始排序
  std::sort(p_paths.begin(), p_paths.end(),
            [](const FSys::path& k_l, const FSys::path& k_r) { return k_l.stem() < k_r.stem(); });
}

void video_sequence::connect_video(const FSys::path& path, const long_term_ptr& in_ptr) const {
  //验证输出文件
  if (!FSys::exists(path.parent_path()))
    FSys::create_directories(path.parent_path());

  auto k_video_input   = cv::VideoCapture{};
  const static cv::Size k_size{1920, 1080};
  auto k_video_out     = cv::VideoWriter{path.generic_string(),
                                     cv::VideoWriter::fourcc('D', 'I', 'V', 'X'),
                                     25,
                                     k_size};
  auto k_image         = cv::Mat{};
  auto k_image_resized = cv::Mat{};
  const auto k_len = p_paths.size();

  for (const auto& path : p_paths) {
    if (k_video_input.open(path.generic_string())) {
      //获得总帧数
      std::size_t k_frame_count = k_video_input.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT);

      while (k_video_input.read(k_image)) {
        std::size_t k_frame = k_video_input.get(cv::VideoCaptureProperties::CAP_PROP_POS_FRAMES);
        if (k_image.cols != k_size.width || k_image.rows != k_size.height)
          cv::resize(k_image, k_image_resized, k_size);
        else
          k_image_resized = k_image;

        k_video_out << k_image_resized;
        if (in_ptr)
          in_ptr->sig_progress(rational_int{1, k_frame_count * k_len});
      }
    } else {
      throw doodle_error("不支持的格式");
    }
  }
  if (in_ptr) {
    in_ptr->sig_message_result(fmt::format("完成视频 {} \n", path), long_term::warning);
    in_ptr->sig_finished();
  }
}

std::string video_sequence::set_shot_and_eps(const shot_ptr& in_shot, const episodes_ptr& in_episodes) {
  if (in_shot && in_episodes) {
    p_name = fmt::format("{}_{}.mp4", in_episodes->str(), in_shot->str());
  } else if (in_shot) {
    p_name = fmt::format("{}.mp4", in_shot->str());
  } else if (in_episodes) {
    p_name = fmt::format("{}.mp4", in_episodes->str());
  }
  return p_name;
}

video_sequence_async::video_sequence_async()
    : p_video(),
      p_backup_out_path() {
}
std::shared_ptr<video_sequence> video_sequence_async::set_video_list(const std::vector<FSys::path>& paths) {
  p_video           = new_object<video_sequence>(paths);
  p_backup_out_path = paths.empty() ? FSys::path{} : paths.front();
  return p_video;
}
long_term_ptr video_sequence_async::connect_video(const FSys::path& path) const {
  //验证输出路径
  auto k_out_path = p_backup_out_path.parent_path() /
                    core_set::getSet().get_uuid_str().append(".mp4");
  if (!path.empty())
    k_out_path = path;

  auto k_term = new_object<long_term>();
  auto k_fut  = doodle_lib::Get().get_thread_pool()->enqueue(
      [self = p_video, k_out_path, k_term]() { self->connect_video(k_out_path, k_term); });
  k_term->p_list.push_back(std::move(k_fut));
  return k_term;
}
}  // namespace doodle
