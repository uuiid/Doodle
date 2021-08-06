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
  long_term_ptr p_term;

 public:
  VideoSequence(decltype(p_paths) paths);

  void connectVideo(const FSys::path& path = {});
  long_term_ptr connectVideo_asyn(const FSys::path& path = {});
};

}  // namespace doodle
