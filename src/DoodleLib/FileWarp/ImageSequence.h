#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/threadPool/LongTerm.h>

#include <boost/signals2.hpp>
namespace doodle {
class DOODLELIB_API ImageSequence : public LongTerm {
  std::vector<FSys::path> p_paths;
  std::string p_Text;
  int p_eps;
  int p_shot;
  std::string p_shot_ab;

  static std::string clearString(const std::string& str);

  bool seanDir(const FSys::path& dir);
  void seanInfo();

 public:
  explicit ImageSequence(std::vector<FSys::path> path, const std::string& text = {});
  explicit ImageSequence(const FSys::path& path_dir, const std::string& text = {});

  bool hasSequence();

  [[nodiscard]] FSys::path getDir() const;

  std::tuple<int, int, std::string> getEpisodesAndShot();
  std::string getEpisodesAndShot_str();
  void setText(const std::string& text);
  void createVideoFile(const FSys::path& out_file);

  boost::signals2::signal<void(float)> stride;
};

using ImageSequencePtr = std::shared_ptr<ImageSequence>;

class DOODLELIB_API ImageSequenceBatch : public LongTerm {
  std::vector<FSys::path> p_paths;
  std::vector<ImageSequencePtr> p_imageSequences;

 public:
  explicit ImageSequenceBatch(decltype(p_paths) dirs);
  explicit ImageSequenceBatch(decltype(p_imageSequences) imageSequences);
  void batchCreateSequence(const FSys::path& out_files = {}) const;

};

}  // namespace doodle
