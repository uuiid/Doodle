#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/threadPool/long_term.h>

namespace doodle {
/**
 * @brief 连接视频的opencv的包装类
 *
 */
class DOODLELIB_API VideoSequence
    : public std::enable_shared_from_this<VideoSequence> {
 private:
  std::vector<FSys::path> p_paths;


 public:
  VideoSequence(std::vector<FSys::path> paths);

  void connectVideo(const FSys::path& path,const long_term_ptr& in_ptr) const;

};

class DOODLELIB_API video_sequence_async : public details::no_copy {
  std::shared_ptr<VideoSequence> p_video;
  FSys::path p_backup_out_path;
 public:
  video_sequence_async();
  void set_video_list(const std::vector<FSys::path>& paths);
  long_term_ptr connect_video(const FSys::path& path = {}) const;
};

}  // namespace doodle
