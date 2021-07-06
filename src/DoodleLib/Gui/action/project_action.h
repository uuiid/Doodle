//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>
namespace doodle {
class DOODLELIB_API actn_create_project : public action {

 public:
  actn_create_project();
  /**
   * @param in_any 喂入的是 std::tuple<std::string,FSys::path> 值
   * @param in_factory 这个是创建项目的工厂指针
   */
  explicit actn_create_project(std::any&& in_any);
  /**
   * @brief std::any 喂入的是 <std::tuple<std::string,FSys::path>  值
   * @param in_data 输入的是 project 数据
   */
  void run(const MetadataPtr& in_data) override;
};

class DOODLELIB_API actn_delete_project : public action {
 public:
  actn_delete_project();
  /**
   * @param in_any 这个不需要任何的输入
   */
  explicit actn_delete_project(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};

class DOODLELIB_API actn_rename_project : public action {
 public:
  actn_rename_project();
  /**
 * @param in_any 需要项目 std::string 名称
 */
  explicit actn_rename_project(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};

class DOODLELIB_API actn_setpath_project : public action {
 public:
  actn_setpath_project();
  /**
   * @param in_any 需要项目路径 FSys::path
   */
  explicit actn_setpath_project(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};

//class DOODLELIB_API set_str_project_action : public action {
// public:
//  set_str_project_action();
//    /**
//   * @param in_any  需要项目短名称 std::string
//   */
//  explicit set_str_project_action(std::any&& in_any);
//  set_str_project_action();
//  void run(const MetadataPtr& in_data) override;
//};
}  // namespace doodle
