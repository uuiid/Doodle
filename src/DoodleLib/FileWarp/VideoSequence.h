#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/threadPool/LongTerm.h>

namespace doodle {
class DOODLELIB_API VideoSequence : public LongTerm {
 private:
  std::vector<FSys::path> p_paths;

 public:
  VideoSequence(decltype(p_paths) paths);

  void connectVideo(const FSys::path& path = {});
};

}  // namespace doodle
