//
// Created by TD on 2021/6/21.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>
namespace doodle {
/**
 * @brief 这个类可以直接上传所有输入的文件夹和文件
 * @param in_paths 需要 std::vector<FSys::path>
 */
class actn_up_paths : public action_indirect<action::arg_paths> {
 public:
  actn_up_paths();

  /**
   * @brief 这里我们需要 std::vector<FSys::path> 传入
   * @param in_paths 需要 std::vector<FSys::path> 传入
   */
  explicit actn_up_paths(std::any && in_paths);

  void run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;

};

}  // namespace doodle
