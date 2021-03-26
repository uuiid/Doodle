#include <lib/kernel/ExeWarp/FFmpegWarp.h>

#include <lib/kernel/BoostUuidWarp.h>
#include <lib/kernel/MotionSetting.h>
#include <lib/kernel/Exception.h>

#include <opencv2/video.hpp>
#include <opencv2/opencv.hpp>

namespace doodle::motion::kernel {

FFmpegWarp::FFmpegWarp()
    : p_file(FSys::temp_directory_path() / "doodle" /
             (boost::uuids::to_string(MotionSetting::Get().random_generator()) + ".mp4")) {
}
FFmpegWarp::~FFmpegWarp() {
  FSys::remove(p_file);
}

bool FFmpegWarp::imageToVideo(const std::vector<std::shared_ptr<FSys::path>> &imagepaths,
                              const FSys::path &videoPath,
                              const std::string &subtitles) const {
  {
    auto video = cv::VideoWriter{p_file.lexically_normal().generic_string(),
                                 cv::VideoWriter::fourcc('D', 'I', 'V', 'X'),
                                 25,
                                 cv::Size(1280, 720)};
    cv::Mat image;

    for (auto &&item : imagepaths) {
      image = cv::imread(item->generic_string());
      video << image;
    }
  }
  if (!FSys::exists(p_file)) throw NotFileError("无法生成文件");

  return FSys::copy_file(p_file, videoPath, FSys::copy_options::update_existing);
}

}  // namespace doodle::motion::kernel