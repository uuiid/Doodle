#pragma once
#include <corelib/core_global.h>

namespace doodle {
class CORE_API ImageSequence {
 private:
  std::vector<FSys::path> p_paths;
  std::string p_Text;

  static std::string clearString(const std::string& str);

 public:
  ImageSequence(decltype(p_paths) path, decltype(p_Text) text = {});
  ImageSequence(FSys::path path_dir, decltype(p_Text) text = {});

  bool hasSequence();

  void setText(const std::string& text);
  void createVideoFile(const FSys::path& out_file);
};

}  // namespace doodle