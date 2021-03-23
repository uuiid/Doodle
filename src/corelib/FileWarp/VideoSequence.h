#pragma once
#include <corelib/core_global.h>

namespace doodle {
class CORE_API VideoSequence {
 private:
  std::vector<FSys::path> p_paths;

 public:
  VideoSequence(decltype(p_paths) paths);

  void connectVideo(const FSys::path& path);
};

}  // namespace doodle