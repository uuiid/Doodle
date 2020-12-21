/*
 * @Author: your name
 * @Date: 2020-10-28 14:09:08
 * @LastEditTime: 2020-12-01 15:13:26
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\exeWrap\ffmpegWrap.h
 */
//
// Created by teXiao on 2020/10/28.
//

#include "core_global.h"
#include <boost/filesystem/path.hpp>

CORE_NAMESPACE_S

class CORE_API ffmpegWrap : public boost::noncopyable_::noncopyable {
 public:
  explicit ffmpegWrap();

  bool imageToVideo(const std::vector<QString> &image_path,
                    const QString &videoPath,
                    const QString &subtitles) const;
  bool imageToVideo(const dpathList &image_path,
                    const dpath &videoPath,
                    const std::string &subtitles) const;
  bool convertToVideo(const dpath &in_videoPath, const dpath &out_videoPath, const std::string &subtitles) const;

  bool connectVideo(const dpathList &in_videoPath, const dpath &out_videoPath);

 private:
  std::shared_ptr<boost::filesystem::path> p_tmp_file_;

 private:
  bool runFFmpeg(const std::string &command) const;
};

CORE_NAMESPACE_E
