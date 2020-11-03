//
// Created by teXiao on 2020/10/28.
//

#include "core_global.h"
#include <boost/filesystem/path.hpp>

CORE_NAMESPACE_S

class CORE_EXPORT ffmpegWrap {
 public:
  explicit ffmpegWrap(const std::string& path);

  bool imageToVideo(const std::vector<QString> &image_path,
                    const QString &videoPath,
                    const QString &subtitles) const;
  bool imageToVideo(const std::vector<std::string> &image_path,
                    const std::string &videoPath,
                    const std::string &subtitles) const;
  bool convertToVideo(const std::string &in_videoPath, const std::string &out_videoPath, const std::string &subtitles) const;
 private:
  std::shared_ptr<boost::filesystem::path> p_path_;
  std::shared_ptr<boost::filesystem::path> p_tmp_file_;
};

CORE_NAMESPACE_E
