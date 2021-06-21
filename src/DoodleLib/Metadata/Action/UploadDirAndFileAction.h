//
// Created by TD on 2021/6/21.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/Action/Action.h>
namespace doodle {
/**
 * @brief 这个类可以直接上传所有输入的文件夹和文件
 * @param in_paths 需要 std::vector<FSys::path>
 */
class UploadDirAndFileAction : public Action {
 public:
  UploadDirAndFileAction();

  /**
   * @brief 这里我们需要 std::vector<FSys::path> 传入
   * @param in_paths 需要 std::vector<FSys::path> 传入
   */
  explicit UploadDirAndFileAction(std::any && in_paths);

  std::string class_name() override;


  void run(const MetadataPtr& in_data) override;

  void operator()(const MetadataPtr& in_data) override;
};

}  // namespace doodle
