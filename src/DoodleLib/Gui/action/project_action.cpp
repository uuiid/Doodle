//
// Created by TD on 2021/6/28.
//

#include "project_action.h"

#include <Logger/Logger.h>
#include <Metadata/MetadataFactory.h>
#include <Metadata/Project.h>
#include <core/CoreSet.h>

namespace doodle {

long_term_ptr actn_create_project::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_val = sig_get_arg().value();

  auto prj = std::make_shared<Project>(k_val.prj_path, k_val.name);
  MetadataFactoryPtr k_f{};
  if (in_data)
    k_f = in_data->getMetadataFactory();
  else if (in_parent) {
    k_f = in_parent->getMetadataFactory();
  } else {
    k_f = std::make_shared<MetadataFactory>();
  }

  prj->updata_db(k_f);

  CoreSet::getSet().p_project_vector.push_back_sig(prj);
  return {};
}
actn_create_project::actn_create_project() {
  p_name = "创建项目";
}

actn_delete_project::actn_delete_project() {
  p_name = "删除项目";
}
long_term_ptr actn_delete_project::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_prj = std::dynamic_pointer_cast<Project>(in_data);

  auto& k_prj_v = CoreSet::getSet().p_project_vector;
  in_data->deleteData(in_data->getMetadataFactory());
  k_prj_v.erase_sig(k_prj);
  return {};
}

actn_rename_project::actn_rename_project() {
  p_name = "重命名项目";
}
long_term_ptr actn_rename_project::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_str = sig_get_arg().value().date;
  auto k_prj = std::dynamic_pointer_cast<Project>(in_data);
  k_prj->setName(k_str);
  k_prj->updata_db(k_prj->getMetadataFactory());
  return {};
}

actn_setpath_project::actn_setpath_project() {
  p_name = "设置路径";
}
long_term_ptr actn_setpath_project::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_path = sig_get_arg().value().date;
  auto k_prj  = std::dynamic_pointer_cast<Project>(in_data);
  k_prj->setPath(k_path);
  k_prj->updata_db(k_prj->getMetadataFactory());
  return {};
}

// set_str_project_action::set_str_project_action() {
//   p_name = "设置英文名称";
// }
// long_term_ptr  set_str_project_action::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
//   auto k_str = std::any_cast<std::string>();
//   auto k_prj = std::dynamic_pointer_cast<Project>(in_data);
//   k_prj->set(k_str);
//   k_prj->updata_db(k_prj->getMetadataFactory());
//  return {};
// }
}  // namespace doodle
