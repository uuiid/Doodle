//
// Created by TD on 2021/7/6.
//

#pragma once
#include <Doodlelib/DoodleLib_fwd.h>
#include <Doodlelib/Gui/action/action.h>

namespace doodle {
namespace action_arg {
class DOODLELIB_API arg_image_to_voide : public action_arg::_arg {
 public:
  arg_image_to_voide() = default;
  arg_image_to_voide(std::vector<FSys::path>& in_paths, FSys::path& in_out_path)
      : image_list(in_paths),
        out_file(in_out_path){};
  /**
   * @brief 图片目录列表, 图片目录列表是一系列填充序列图的文件夹
   * 
   */
  std::vector<FSys::path> image_list;
  /**
   * @brief 输出路径， 没有文件名称， 文件名称由程序生成
   * 
   */
  FSys::path out_file;
};

}  // namespace action_arg

class DOODLELIB_API actn_image_to_movie : public action_indirect<action_arg::arg_image_to_voide> {
  FSys::path p_video_path;
  ImageSequencePtr p_image_sequence;

 public:
  using arg_ = action_arg::arg_image_to_voide;

  /**
   * @brief 将一个文件夹中的图片连接为视频
   * 
   */
  actn_image_to_movie();
  virtual bool is_async() override;

  FSys::path get_video_path() const;

  bool is_accept(const arg_& in_any) override;
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};

class DOODLELIB_API actn_image_to_move_up : public actn_composited<action_arg::arg_image_to_voide> {
  std::shared_ptr<actn_image_to_movie> p_image_action;
  std::shared_ptr<actn_up_paths> p_up_path;

 public:
  using arg_ = action_arg::arg_image_to_voide;
  virtual bool is_accept(const arg_& in_any) override;
  actn_image_to_move_up();
  virtual bool is_async() override;
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};

}  // namespace doodle
