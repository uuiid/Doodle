//
// Created by TD on 2021/6/28.
//

#include "project_action.h"

#include <Logger/Logger.h>
#include <Metadata/Project.h>
#include <core/CoreSet.h>
namespace doodle {
actn_create_project::actn_create_project(std::any&& in_any)
    : action(std::move(in_any)) {
  p_name = "创建项目";
}

void actn_create_project::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();

  if (!p_any.has_value()) {
    DOODLE_LOG_WARN("没有发现值")
    return;
  }
  auto [k_s, k_p] = std::any_cast<std::tuple<std::string, FSys::path> >(p_any);

  auto prj = std::make_shared<Project>(k_p, k_s);
  prj->updata_db(in_data->getMetadataFactory());

  CoreSet::getSet().p_project_vector.push_back_sig(prj);
}
actn_create_project::actn_create_project() {
  p_name = "创建项目";
}

actn_delete_project::actn_delete_project(std::any&& in_any)
    : action(std::move(in_any)) {
  p_name = "删除项目";
}
actn_delete_project::actn_delete_project()
    : action() {
  p_name = "删除项目";
}
void actn_delete_project::run(const MetadataPtr& in_data) {
  auto k_prj = std::dynamic_pointer_cast<Project>(in_data);

  auto& k_prj_v = CoreSet::getSet().p_project_vector;
  k_prj_v.erase_sig(k_prj);

  in_data->deleteData(in_data->getMetadataFactory());
}

actn_rename_project::actn_rename_project() {
  p_name = "重命名项目";
}
actn_rename_project::actn_rename_project(std::any&& in_any) : action(std::move(in_any)) {
  p_name = "重命名项目";
}
void actn_rename_project::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();

  if (!p_any.has_value()) {
    DOODLE_LOG_WARN("没有发现值")
    return;
  }

  auto k_str = std::any_cast<std::string>(p_any);
  auto k_prj = std::dynamic_pointer_cast<Project>(in_data);
  k_prj->setName(k_str);
  k_prj->updata_db(k_prj->getMetadataFactory());
}

actn_setpath_project::actn_setpath_project() {
  p_name = "设置路径";
}
actn_setpath_project::actn_setpath_project(std::any&& in_any) : action(std::move(in_any)) {
  p_name = "设置路径";
}
void actn_setpath_project::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();

  if (!p_any.has_value()) {
    DOODLE_LOG_WARN("没有发现值")
    return;
  }

  auto k_path = std::any_cast<FSys::path>(p_any);
  auto k_prj  = std::dynamic_pointer_cast<Project>(in_data);
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
