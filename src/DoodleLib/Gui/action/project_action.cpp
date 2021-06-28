//
// Created by TD on 2021/6/28.
//

#include "project_action.h"

#include <Logger/Logger.h>
#include <Metadata/Project.h>
#include <core/MetadataSet.h>
namespace doodle {
create_project_action::create_project_action(std::any&& in_any, MetadataFactoryPtr in_factory)
    : action(std::move(in_any)),
      p_factory(std::move(in_factory)) {
  p_name = "创建项目";
}

void create_project_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();

  if (!p_any.has_value())
    DOODLE_LOG_WARN("没有发现值")
  auto [k_s, k_p] = std::any_cast<std::tuple<std::string, FSys::path> >(p_any);

  auto prj = std::make_shared<Project>(k_p, k_s);
  prj->updata_db(p_factory);
  MetadataSet::Get().installProject(prj);
}

delete_project_action::delete_project_action(std::any&& in_any)
    : action(std::move(in_any)) {
  p_name = "删除项目";
}
delete_project_action::delete_project_action()
    : action() {
  p_name = "删除项目";
}
void delete_project_action::run(const MetadataPtr& in_data) {
  auto k_prj = std::dynamic_pointer_cast<Project>(in_data);

  MetadataSet::Get().deleteProject(k_prj.get());
  in_data->deleteData(in_data->getMetadataFactory());
}

rename_project_action::rename_project_action() {
  p_name = "重命名项目";
}
rename_project_action::rename_project_action(std::any&& in_any) : action(std::move(in_any)) {
  p_name = "重命名项目";
}
void rename_project_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();

  auto k_str = std::any_cast<std::string>(p_any);
  auto k_prj = std::dynamic_pointer_cast<Project>(in_data);
  k_prj->setName(k_str);
  k_prj->updata_db(k_prj->getMetadataFactory());
}

setpath_project_action::setpath_project_action() {
  p_name = "设置路径";
}
setpath_project_action::setpath_project_action(std::any&& in_any) : action(std::move(in_any)) {
  p_name = "设置路径";
}
void setpath_project_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();

  auto k_path = std::any_cast<FSys::path>(p_any);
  auto k_prj = std::dynamic_pointer_cast<Project>(in_data);
  k_prj->setPath(k_path);
  k_prj->updata_db(k_prj->getMetadataFactory());
}

//set_str_project_action::set_str_project_action() {
//  p_name = "设置英文名称";
//}
//set_str_project_action::set_str_project_action(std::any&& in_any) : action(std::move(in_any)) {
//  p_name = "设置英文名称";
//}
//void set_str_project_action::run(const MetadataPtr& in_data) {
//  if (!p_any.has_value())
//    p_any = sig_get_input().value();
//
//
//  auto k_str = std::any_cast<std::string>(p_any);
//  auto k_prj = std::dynamic_pointer_cast<Project>(in_data);
//  k_prj->set(k_str);
//  k_prj->updata_db(k_prj->getMetadataFactory());
//}
}  // namespace doodle
