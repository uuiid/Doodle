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
class actn_up_paths : public action_indirect<action_arg::arg_paths> {
  rpc_trans::trans_file_ptr p_tran;

 public:
  actn_up_paths();
  using arg_ = action_arg::arg_paths;
  bool is_async() override;
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};

class actn_create_ass_up_paths : public action_indirect<action_arg::arg_paths> {
  actn_up_paths_ptr p_up;

 public:
  actn_create_ass_up_paths();
  using arg_ = action_arg::arg_paths;

  bool is_async() override;
  virtual long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};
}  // namespace doodle
