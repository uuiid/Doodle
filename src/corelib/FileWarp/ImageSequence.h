#pragma once
#include <corelib/core_global.h>

namespace doodle {
class ImageSequence {
 private:
  std::vector<pathPtr> p_paths;

 public:
  ImageSequence(decltype(p_paths) path);

  void createVideoFile(const FSys::path &out_file);
};

}  // namespace doodle