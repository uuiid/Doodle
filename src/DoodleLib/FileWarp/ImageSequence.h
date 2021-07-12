#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/threadPool/long_term.h>

#include <boost/signals2.hpp>
namespace doodle {
class DOODLELIB_API ImageSequence : public long_term {
  std::vector<FSys::path> p_paths;
  std::string p_Text;
  long_term_ptr p_long_sig;

  static std::string clearString(const std::string& str);

  bool seanDir(const FSys::path& dir);

 public:
  explicit ImageSequence(const FSys::path& path_dir, const std::string& text = {});

  bool hasSequence();

  void setText(const std::string& text);
  void createVideoFile(const FSys::path& out_file);
  long_term_ptr create_video_asyn(const FSys::path& out_file);


  boost::signals2::signal<void(float)> stride;
};

using ImageSequencePtr = std::shared_ptr<ImageSequence>;

//class DOODLELIB_API ImageSequenceBatch : public LongTerm {
//  std::vector<FSys::path> p_paths;
//  std::vector<ImageSequencePtr> p_imageSequences;
//
// public:
//  explicit ImageSequenceBatch(decltype(p_paths) dirs);
//  explicit ImageSequenceBatch(decltype(p_imageSequences) imageSequences);
//  void batchCreateSequence(const FSys::path& out_files = {}) const;
//
//};

}  // namespace doodle
