#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/thread_pool/long_term.h>

#include <boost/signals2.hpp>
namespace doodle {

namespace details {
class DOODLELIB_API image_file {
  FSys::path p_file;
  std::vector<std::int32_t> p_list;

  std::int32_t p_frame;
  std::int64_t p_index;

  const std::vector<std::int32_t>& extract_num();

 public:
  image_file() : p_frame(-1), p_index(-1), p_list(), p_file(){};
  explicit image_file(const FSys::path& in_path) : image_file() {
    set_path(in_path);
  };

  void set_path(const FSys::path& in_);

  std::int32_t get_frame() const;

  bool speculate_frame(const image_file& in);
  bool next(image_file& in) const;

  bool operator==(const image_file& in_rhs) const;
  bool operator!=(const image_file& in_rhs) const;

  bool operator<(const image_file& in_rhs) const;
  bool operator>(const image_file& in_rhs) const;
  bool operator<=(const image_file& in_rhs) const;
  bool operator>=(const image_file& in_rhs) const;
  operator bool() const;
};
using image_file_ptr = std::shared_ptr<image_file>;
}  // namespace details

class DOODLELIB_API image_sequence
    : public std::enable_shared_from_this<image_sequence> {
  std::vector<FSys::path> p_paths;
  std::string p_Text;
  FSys::path p_out_path;
  std::string p_name;
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
  image_sequence();
  explicit image_sequence(const FSys::path& path_dir, const std::string& text = {});

  bool has_sequence();
  void set_path(const FSys::path& dir);
  void set_path(const std::vector<FSys::path>& in_images);
  void set_text(const std::string& text);
  void set_out_dir(const FSys::path& out_dir);
  FSys::path get_out_path() const;
  static std::string show_str(const std::vector<FSys::path>& in_images);
  /**
   * @brief 使用这个可以将镜头和和集数还有水印， 路径等一起设置完成
   *
   * @param in_shot 要使用的镜头元数据
   * @param in_episodes 要使用的集数元数据
   * @return std::string 生成的水印
   */
  std::string set_shot_and_eps(const shot_ptr& in_shot, const episodes_ptr& in_episodes);
  void create_video(const long_term_ptr& in_ptr);

  static bool is_image_sequence(const std::vector<FSys::path>& in_file_list);
};

class DOODLELIB_API image_sequence_async : public details::no_copy {
  image_sequence_ptr p_image_sequence;

 public:
  image_sequence_async();
  /**
   * @brief 创建一个图片序列对象
   * @warning 需要手动的创建一个输出目录和输出文件名称
   * @param image_dir 图片序列所在的文件夹
   */
  image_sequence_ptr set_path(const FSys::path& image_dir);
  /**
   * @brief 创建一个图片序列对象
   *
   * @warning 需要手动的创建一个输出目录和输出文件名称
   *
   * @param image_dir 图片序列
   */
  image_sequence_ptr set_path(const std::vector<FSys::path>& image_path_list);
  /**
   * @brief 创建一个图片序列对象
   *
   * @warning 这个是不需要调用创建视频时指定输出帧的,
   *
   * @param image_dir 图片序列
   */
  image_sequence_ptr set_path(const assets_path_vector_ptr& in_path);

  long_term_ptr create_video(const FSys::path& out_file);
  long_term_ptr create_video();

};
}  // namespace doodle
