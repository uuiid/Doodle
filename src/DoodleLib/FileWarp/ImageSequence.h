#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/threadPool/long_term.h>

#include <boost/signals2.hpp>
namespace doodle {
class DOODLELIB_API ImageSequence
    : public std::enable_shared_from_this<ImageSequence> {
  std::vector<FSys::path> p_paths;
  std::string p_Text;
  FSys::path p_out_path;

  struct asyn_arg {
    std::vector<FSys::path> paths;
    std::string Text;
    long_term_ptr long_sig;

    FSys::path out_path;
  };
  using asyn_arg_ptr = std::shared_ptr<asyn_arg>;
  static void create_video(const asyn_arg_ptr& in_arg);
  static std::string clearString(const std::string& str);

  bool seanDir(const FSys::path& dir);

 public:
  ImageSequence();
  explicit ImageSequence(const FSys::path& path_dir, const std::string& text = {});

  bool hasSequence();
  void set_path(const FSys::path& dir);
  void set_path(const std::vector<FSys::path>& in_images);
  void setText(const std::string& text);
  /**
   * @brief 使用这个可以将镜头和和集数还有水印， 路径等一起设置完成
   *
   * @param in_shot 要使用的镜头元数据
   * @param in_episodes 要使用的集数元数据
   * @return std::string 生成的水印
   */
  std::string set_shot_and_eps(const ShotPtr& in_shot, const EpisodesPtr& in_episodes);
  void create_video(const FSys::path& out_file, const long_term_ptr& in_ptr);
};

class DOODLELIB_API image_sequence_async : public details::no_copy {
  ImageSequencePtr p_image_sequence;

 public:
  image_sequence_async();
  void set_path(const FSys::path& image_dir);
  void set_path(const std::vector<FSys::path>& image_path_list);
  long_term_ptr create_video(const FSys::path& out_file);
};
// class DOODLELIB_API ImageSequenceBatch : public LongTerm {
//   std::vector<FSys::path> p_paths;
//   std::vector<ImageSequencePtr> p_imageSequences;
//
//  public:
//   explicit ImageSequenceBatch(decltype(p_paths) dirs);
//   explicit ImageSequenceBatch(decltype(p_imageSequences) imageSequences);
//   void batchCreateSequence(const FSys::path& out_files = {}) const;
//
// };

}  // namespace doodle
