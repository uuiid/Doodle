//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>

namespace doodle {

class DOODLELIB_API actn_assfile_create : public action_indirect<action::arg_str> {
  AssetsFilePtr _assets_file;

 public:
  using arg = arg_str;

  actn_assfile_create();
  AssetsFilePtr get_result();
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};

class DOODLELIB_API actn_assfile_add_com : public action_indirect<action::arg_str> {
 public:
  using arg = arg_str;
  actn_assfile_add_com();
  explicit actn_assfile_add_com(std::any&& in_any);
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};

namespace action_arg {
class arg_time : public action_arg::_arg {
 public:
  arg_time() = default;
  explicit arg_time(TimeDurationPtr in_) : time(std::move(in_)){};
  TimeDurationPtr time;
};

}  // namespace action_arg

class DOODLELIB_API actn_assfile_datetime : public action_indirect<action_arg::arg_time> {
 public:
  using arg = action_arg::arg_time;
  actn_assfile_datetime();
  explicit actn_assfile_datetime(std::any&& in_any);
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};
class DOODLELIB_API actn_assfile_delete : public action_indirect<action_arg::arg_null> {
 public:
  using arg = action_arg::arg_null;

  actn_assfile_delete();
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};
class DOODLELIB_API actn_assdile_attr_show : public action_indirect<action_arg::arg_null>{
 public:
  using arg = action_arg::arg_null;
  actn_assdile_attr_show();
  virtual bool is_async() override;
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};

}  // namespace doodle
