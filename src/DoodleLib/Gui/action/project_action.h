//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>
namespace doodle {
namespace action_arg {
class DOODLELIB_API arg_prj : public action_arg::_arg {
 public:
  arg_prj() = default;
  arg_prj(FSys::path& in_p, std::string& in_s)
      : prj_path(in_p),
        name(in_s){};

  FSys::path prj_path;
  std::string name;
};

}  // namespace action_arg

class DOODLELIB_API actn_create_project : public action_indirect<action_arg::arg_prj> {
 public:
  actn_create_project();

  using arg = action_arg::arg_prj;
  /**
   * @param in_any 喂入的是 std::tuple<std::string,FSys::path> 值
   * @param in_factory 这个是创建项目的工厂指针
   */
  explicit actn_create_project(std::any&& in_any);
  /**
   * @brief std::any 喂入的是 <std::tuple<std::string,FSys::path>  值
   * @param in_data 输入的是 project 数据
   */
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};

class DOODLELIB_API actn_delete_project : public action_indirect<action_arg::arg_null> {
 public:
  using arg = action_arg::arg_null;
  actn_delete_project();
  /**
   * @param in_any 这个不需要任何的输入
   */
  explicit actn_delete_project(std::any&& in_any);
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};

class DOODLELIB_API actn_rename_project : public action_indirect<action_arg::arg_str> {
 public:
  actn_rename_project();

  using arg = action_arg::arg_str;

  /**
 * @param in_any 需要项目 std::string 名称
 */
  explicit actn_rename_project(std::any&& in_any);
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};

class DOODLELIB_API actn_setpath_project : public action_indirect<action_arg::arg_path> {
 public:
  using arg = action_arg::arg_path;

  actn_setpath_project();
  /**
   * @param in_any 需要项目路径 FSys::path
   */
  explicit actn_setpath_project(std::any&& in_any);
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};

//class DOODLELIB_API set_str_project_action : public action {
// public:
//  set_str_project_action();
//    /**
//   * @param in_any  需要项目短名称 std::string
//   */
//  explicit set_str_project_action(std::any&& in_any);
//  set_str_project_action();
//  long_term_ptr  run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
//};
}  // namespace doodle
