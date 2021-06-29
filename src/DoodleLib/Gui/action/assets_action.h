//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>

namespace doodle {

class DOODLELIB_API assset_create_action : public action {
  MetadataFactoryPtr p_factory;

 public:
  assset_create_action();
  explicit assset_create_action(std::any&& in_any, MetadataFactoryPtr in_factory);
  void run(const MetadataPtr& in_data) override;
};

class DOODLELIB_API episode_create_action : public action {
  MetadataFactoryPtr p_factory;

 public:
  episode_create_action();
  explicit episode_create_action(std::any&& in_any, MetadataFactoryPtr in_factory);
  void run(const MetadataPtr& in_data) override;
};
class DOODLELIB_API shot_create_action : public action {
  MetadataFactoryPtr p_factory;

 public:
  shot_create_action();
  explicit shot_create_action(std::any&& in_any, MetadataFactoryPtr in_factory);
  void run(const MetadataPtr& in_data) override;
};

class DOODLELIB_API assets_delete_action : public action {
 public:
  assets_delete_action();
  explicit assets_delete_action(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};

class DOODLELIB_API episode_set_action : public action {
 public:
  episode_set_action();
  explicit episode_set_action(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};
class DOODLELIB_API shot_set_action : public action {
 public:
  shot_set_action();
  explicit shot_set_action(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};
class DOODLELIB_API shotab_set_action : public action {
 public:
  shotab_set_action();
  explicit shotab_set_action(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};
class DOODLELIB_API assets_setname_action : public action {
 public:
  assets_setname_action();
  explicit assets_setname_action(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};
}  // namespace doodle
