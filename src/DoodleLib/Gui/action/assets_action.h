//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>

namespace doodle {

class DOODLELIB_API actn_assets_create : public action_indirect<action_arg::arg_str> {
 public:
  using arg = action_arg::arg_str;
  actn_assets_create();
  explicit actn_assets_create(std::any&& in_any);
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};

class DOODLELIB_API actn_episode_create : public action_indirect<action_arg::arg_int> {
 public:
  using arg = action_arg::arg_int;

  actn_episode_create();
  explicit actn_episode_create(std::any&& in_any);
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};
class DOODLELIB_API actn_shot_create : public action_indirect<action_arg::arg_int> {
 public:
  using arg = action_arg::arg_int;

  actn_shot_create();
  explicit actn_shot_create(std::any&& in_any);
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};

class DOODLELIB_API actn_assets_delete : public action_indirect<action_arg::arg_null> {
 public:
  using arg = action_arg::arg_null;
  actn_assets_delete();
  explicit actn_assets_delete(std::any&& in_any);
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};

class DOODLELIB_API actn_episode_set : public action_indirect<action_arg::arg_int> {
 public:
  using arg = action_arg::arg_int;
  actn_episode_set();
  explicit actn_episode_set(std::any&& in_any);
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};
class DOODLELIB_API actn_shot_set : public action_indirect<action_arg::arg_int> {
 public:
  using arg = action_arg::arg_int;
  actn_shot_set();
  explicit actn_shot_set(std::any&& in_any);
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};
class DOODLELIB_API actn_shotab_set : public action_indirect<action_arg::arg_str> {
 public:
  using arg = action_arg::arg_str;
  actn_shotab_set();
  explicit actn_shotab_set(std::any&& in_any);
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};
class DOODLELIB_API actn_assets_setname : public action_indirect<action_arg::arg_str> {
 public:
  using arg = action_arg::arg_str;
  actn_assets_setname();
  explicit actn_assets_setname(std::any&& in_any);
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};
}  // namespace doodle
