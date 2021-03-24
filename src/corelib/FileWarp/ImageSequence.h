#pragma once
#include <corelib/core_global.h>

#include <boost/signals2.hpp>
namespace doodle {
class CORE_API ImageSequence {
  std::vector<FSys::path> p_paths;
  std::string p_Text;
  int p_eps;
  int p_shot;
  std::string p_shot_ab;

  static std::string clearString(const std::string& str);

  bool seanDir(const FSys::path& dir);
  void seanInfo();

 public:
  ImageSequence(decltype(p_paths) path, decltype(p_Text) text = {});
  ImageSequence(FSys::path path_dir, decltype(p_Text) text = {});

  bool hasSequence();

  FSys::path getDir() const;

  std::tuple<int, int, std::string> getEpisodesAndShot();
  std::string getEpisodesAndShot_str();
  void setText(const std::string& text);
  void createVideoFile(const FSys::path& out_file);

  boost::signals2::signal<void(int)> progress;
  boost::signals2::signal<void(const std::string& message)> messagResult;
  boost::signals2::signal<void()> finished;
};

using ImageSequencePtr = std::shared_ptr<ImageSequence>;

class CORE_API ImageSequenceBatch {
  std::vector<FSys::path> p_paths;
  std::vector<ImageSequencePtr> p_imageSequences;

 public:
  ImageSequenceBatch(decltype(p_paths) dirs);
  ImageSequenceBatch(decltype(p_imageSequences) imageSequences);
  void batchCreateSequence(const FSys::path& out_files = {}) const;

  boost::signals2::signal<void(int)> progress;
  boost::signals2::signal<void(const std::string& message)> messagResult;
  boost::signals2::signal<void()> finished;
};

}  // namespace doodle