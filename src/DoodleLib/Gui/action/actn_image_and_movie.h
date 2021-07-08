//
// Created by TD on 2021/7/6.
//

#pragma once
#include <Doodlelib/DoodleLib_fwd.h>
#include <Doodlelib/Gui/action/action.h>

namespace doodle {
namespace action_arg {
class DOODLELIB_API arg_path : public action::_arg {
 public:
  arg_path() = default;
  arg_path(std::vector<FSys::path>& in_paths, FSys::path& in_out_path)
      : image_list(in_paths),
        out_file(in_out_path){};
  std::vector<FSys::path> image_list;
  FSys::path out_file;
};

}  // namespace action_arg

class DOODLELIB_API actn_image_to_movie : public action_indirect<action_arg::arg_path> {
 public:
  using arg_ = action_arg::arg_path;

  /**
   * @brief 将一个文件夹中的图片连接为视频
   * 
   */
  actn_image_to_movie();

  bool is_accept(const arg_& in_any) override;
  void run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};

}  // namespace doodle
