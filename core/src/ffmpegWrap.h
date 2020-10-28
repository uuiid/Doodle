//
// Created by teXiao on 2020/10/28.
//

#include "core_global.h"
CORE_NAMESPACE_S

class CORE_EXPORT ffmpegWrap{
 public:
  explicit ffmpegWrap (QString  path);


  bool imageToVideo(const std::vector<QString> &image_path,
                    const QString &videoPath,
                    const QString &subtitles) const;
 private:
  QString p_path_;
  std::shared_ptr<QTemporaryFile> p_file_;
};

CORE_NAMESPACE_E
