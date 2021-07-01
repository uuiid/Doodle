//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>
namespace doodle {
class DOODLELIB_API create_project_action : public action {

 public:
  create_project_action();
  /**
   * @param in_any 喂入的是 std::tuple<std::string,FSys::path> 值
   * @param in_factory 这个是创建项目的工厂指针
   */
  explicit create_project_action(std::any&& in_any);
  /**
   * @brief std::any 喂入的是 <std::tuple<std::string,FSys::path>  值
   * @param in_data 输入的是 project 数据
   */
  void run(const MetadataPtr& in_data) override;
};

class DOODLELIB_API delete_project_action : public action {
 public:
  delete_project_action();
  /**
   * @param in_any 这个不需要任何的输入
   */
  explicit delete_project_action(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};

class DOODLELIB_API rename_project_action : public action {
 public:
  rename_project_action();
  /**
 * @param in_any 需要项目 std::string 名称
 */
  explicit rename_project_action(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};

class DOODLELIB_API setpath_project_action : public action {
 public:
  setpath_project_action();
  /**
   * @param in_any 需要项目路径 FSys::path
   */
  explicit setpath_project_action(std::any&& in_any);
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
