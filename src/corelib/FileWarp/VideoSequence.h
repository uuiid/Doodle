#pragma once
#include <corelib/core_global.h>
#include <corelib/threadPool/LongTerm.h>

namespace doodle {
class CORE_API VideoSequence : public LongTerm {
 private:
  std::vector<FSys::path> p_paths;

 public:
  VideoSequence(decltype(p_paths) paths);

  void connectVideo(const FSys::path& path = {});
};

}  // namespace doodle