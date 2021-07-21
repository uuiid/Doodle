//
// Created by TD on 2021/7/13.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>
namespace doodle {

class DOODLELIB_API actn_down_paths : public action_indirect<action_arg::arg_path> {
  rpc_trans::trans_file_ptr p_tran;

 public:
  actn_down_paths();
  using arg_ = action_arg::arg_path;
  virtual bool is_async() override;
  virtual long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};
}  // namespace doodle
