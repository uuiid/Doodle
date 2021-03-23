#pragma once
#include <corelib/core_global.h>

#include <boost/signals2.hpp>
namespace doodle {
class CORE_API ImageSequence {
 private:
  std::vector<FSys::path> p_paths;
  std::string p_Text;

  static std::string clearString(const std::string& str);
  bool p_has_batch;

 public:
  ImageSequence(decltype(p_paths) path, decltype(p_Text) text = {});
  ImageSequence(FSys::path path_dir, decltype(p_Text) text = {});

  bool hasSequence();
  bool seanDir(const FSys::path& dir);

  void setText(const std::string& text);
  void createVideoFile(const FSys::path& out_file);

  static void batchCreateSequence(const std::vector<FSys::path>& dirs);

  boost::signals2::signal<void(int)> progress;
  boost::signals2::signal<void(const std::string& message)> messagResult;
  boost::signals2::signal<void()> finished;
};

}  // namespace doodle