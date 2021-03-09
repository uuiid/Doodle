#pragma once
#include <lib/MotionGlobal.h>

namespace doodle::motion::kernel {
class FFmpegWarp {
 private:
  FSys::path p_file;

  bool runFFmpeg(const std::string &command) const;

 public:
  FFmpegWarp();
  ~FFmpegWarp();
  bool imageToVideo(const std::vector<std::shared_ptr<FSys::path>> &imagepaths,
                    const FSys::path &videoPath,
                    const std::string &subtitles) const;
};

}  // namespace doodle::motion::kernel