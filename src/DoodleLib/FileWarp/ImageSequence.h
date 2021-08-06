#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/threadPool/long_term.h>

#include <boost/signals2.hpp>
namespace doodle {
class DOODLELIB_API ImageSequence : public std::enable_shared_from_this<ImageSequence> {
  std::vector<FSys::path> p_paths;
  std::string p_Text;
  long_term_ptr p_long_sig;

  FSys::path p_out_path;
  static std::string clearString(const std::string& str);

  bool seanDir(const FSys::path& dir);

 public:
  ImageSequence();
  explicit ImageSequence(const FSys::path& path_dir, const std::string& text = {});

  bool hasSequence();
  void set_path(const FSys::path& dir);
  long_term_ptr get_long_term();

  void setText(const std::string& text);
  /**
   * @brief 使用这个可以将镜头和和集数还有水印， 路径等一起设置完成
   * 
   * @param in_shot 要使用的镜头元数据
   * @param in_episodes 要使用的集数元数据
   * @return std::string 生成的水印
   */
  std::string set_shot_and_eps(const ShotPtr& in_shot, const EpisodesPtr& in_episodes);
  void createVideoFile(const FSys::path& out_file = {});
  long_term_ptr create_video_asyn(const FSys::path& out_file = {});

  boost::signals2::signal<void(float)> stride;
};

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
