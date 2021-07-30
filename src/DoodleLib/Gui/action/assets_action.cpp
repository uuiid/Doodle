//
// Created by TD on 2021/6/28.
//

#include "assets_action.h"

#include <Metadata/Metadata_cpp.h>

namespace doodle {

actn_assets_create::actn_assets_create() {
  p_name = "创建类别";
}
long_term_ptr actn_assets_create::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  _arg_type = sig_get_arg().value();
  if (_arg_type.is_cancel)
    return {};
  if (_arg_type.date.empty()) {
    DOODLE_LOG_WARN("没有值")
    return {};
  }
  auto k_a = std::make_shared<Assets>(in_parent, _arg_type.date);
  in_parent->child_item.push_back_sig(k_a);

  k_a->updata_db(in_parent->getMetadataFactory());
  return {};
}

actn_episode_create::actn_episode_create() {
  p_name = "创建集数";
}
long_term_ptr actn_episode_create::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  _arg_type = sig_get_arg().value();
  if (_arg_type.is_cancel)
    return {};

  auto k_item = std::make_shared<Episodes>(in_parent, _arg_type.date);
  in_parent->child_item.push_back_sig(k_item);
  k_item->updata_db(in_parent->getMetadataFactory());
  return {};
}

actn_shot_create::actn_shot_create() {
  p_name = "创建镜头";
}

long_term_ptr actn_shot_create::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  _arg_type = sig_get_arg().value();
  if (_arg_type.is_cancel)
    return {};
  auto k_item = std::make_shared<Shot>(in_parent, _arg_type.date);
  in_parent->child_item.push_back_sig(k_item);
  k_item->updata_db(in_parent->getMetadataFactory());
  return {};
}

actn_assets_delete::actn_assets_delete() {
  p_name = "删除";
}

long_term_ptr actn_assets_delete::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  in_data->deleteData(in_data->getMetadataFactory());
  auto k_p = in_data->getParent();

  k_p->child_item.erase_sig(in_data);
  return {};
}

long_term_ptr actn_episode_set::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  _arg_type = sig_get_arg().value();
  if (_arg_type.is_cancel)
    return {};

  auto k_eps = std::dynamic_pointer_cast<Episodes>(in_data);
  k_eps->setEpisodes(_arg_type.date);
  k_eps->updata_db(k_eps->getMetadataFactory());
  return {};
}
actn_episode_set::actn_episode_set() {
  p_name = "设置集数";
}

long_term_ptr actn_shot_set::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  _arg_type = sig_get_arg().value();
  if (_arg_type.is_cancel)
    return {};

  auto k_item = std::dynamic_pointer_cast<Shot>(in_data);
  k_item->setShot(_arg_type.date);
  k_item->updata_db(k_item->getMetadataFactory());
  return {};
}
actn_shot_set::actn_shot_set() {
  p_name = "设置镜头号";
}

long_term_ptr actn_shotab_set::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  _arg_type = sig_get_arg().value();

  if (_arg_type.is_cancel)
    return {};

  auto k_item = std::dynamic_pointer_cast<Shot>(in_data);
  k_item->setShotAb(_arg_type.date);
  k_item->updata_db(k_item->getMetadataFactory());
  return {};
}
actn_shotab_set::actn_shotab_set() {
  p_name = "设置ab镜头";
}

long_term_ptr actn_assets_setname::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_s    = sig_get_arg().value().date;
  auto k_item = std::dynamic_pointer_cast<Assets>(in_data);
  k_item->setName1(k_s);
  k_item->updata_db(k_item->getMetadataFactory());
  return {};
}
actn_assets_setname::actn_assets_setname() {
  p_name = "设置名称";
}

actn_season_create::actn_season_create() {
  p_name = "创建季数";
}
long_term_ptr actn_season_create::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  _arg_type = sig_get_arg().value();

  if (_arg_type.is_cancel)
    return {};

  auto k_item = std::make_shared<season>(in_parent, _arg_type.date);
  in_parent->child_item.push_back_sig(k_item);
  k_item->updata_db();
  return {};
}
actn_season_set::actn_season_set() {
  p_name = "设置季数";
}
long_term_ptr actn_season_set::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  _arg_type = sig_get_arg().value();
  if (_arg_type.is_cancel)
    return {};

  auto item = std::dynamic_pointer_cast<season>(in_data);
  if (!item)
    return {};
  item->set_season(_arg_type.date);
  item->updata_db();
  return {};
}
}  // namespace doodle
