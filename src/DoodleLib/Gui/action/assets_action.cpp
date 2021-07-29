//
// Created by TD on 2021/6/28.
//

#include "assets_action.h"
#include <Metadata/MetadataFactory.h>

#include <Metadata/Assets.h>
#include <Metadata/Episodes.h>
#include <Metadata/Shot.h>
namespace doodle {

actn_assets_create::actn_assets_create(std::any&& in_any) {
  p_name = "创建类别";
}
actn_assets_create::actn_assets_create() {
  p_name = "创建类别";
}
long_term_ptr actn_assets_create::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_s = sig_get_arg().value().date;
  if (k_s.empty()) {
    DOODLE_LOG_WARN("没有值")
    return {};
  }
  auto k_a = std::make_shared<Assets>(in_parent, k_s);
  in_parent->child_item.push_back_sig(k_a);

  k_a->updata_db(in_parent->getMetadataFactory());
  in_parent->sortChildItems(true);
  return {};
}

actn_episode_create::actn_episode_create(std::any&& in_any) {
  p_name = "创建集数";
}
actn_episode_create::actn_episode_create() {
  p_name = "创建集数";
}
long_term_ptr actn_episode_create::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_i    = sig_get_arg().value().date;
  auto k_item = std::make_shared<Episodes>(in_parent, k_i);
  in_parent->child_item.push_back_sig(k_item);
  k_item->updata_db(in_parent->getMetadataFactory());
  in_parent->sortChildItems(true);
  return {};
}

actn_shot_create::actn_shot_create(std::any&& in_any) {
  p_name = "创建镜头";
}
actn_shot_create::actn_shot_create() {
  p_name = "创建镜头";
}

long_term_ptr actn_shot_create::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_i    = sig_get_arg().value().date;
  auto k_item = std::make_shared<Shot>(in_parent, k_i);
  in_parent->child_item.push_back_sig(k_item);
  k_item->updata_db(in_parent->getMetadataFactory());
  in_parent->sortChildItems(true);
  return {};
}

actn_assets_delete::actn_assets_delete(std::any&& in_any) {
  p_name = "删除";
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

actn_episode_set::actn_episode_set(std::any&& in_any) {
  p_name = "设置集数";
}
long_term_ptr actn_episode_set::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_i   = sig_get_arg().value().date;
  auto k_eps = std::dynamic_pointer_cast<Episodes>(in_data);
  if (k_i == 0)
    return {};
  k_eps->setEpisodes(k_i);
  k_eps->updata_db(k_eps->getMetadataFactory());
  return {};
}
actn_episode_set::actn_episode_set() {
  p_name = "设置集数";
}

actn_shot_set::actn_shot_set(std::any&& in_any) {
  p_name = "设置镜头号";
}
long_term_ptr actn_shot_set::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_i    = sig_get_arg().value().date;
  auto k_item = std::dynamic_pointer_cast<Shot>(in_data);
  if (k_i == 0)
    return {};
  k_item->setShot(k_i);
  k_item->updata_db(k_item->getMetadataFactory());
  return {};
}
actn_shot_set::actn_shot_set() {
  p_name = "设置镜头号";
}

actn_shotab_set::actn_shotab_set(std::any&& in_any) {
  p_name = "设置ab镜头";
}
long_term_ptr actn_shotab_set::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_s    = sig_get_arg().value().date;
  auto k_item = std::dynamic_pointer_cast<Shot>(in_data);
  k_item->setShotAb(k_s);
  k_item->updata_db(k_item->getMetadataFactory());
  return {};
}
actn_shotab_set::actn_shotab_set() {
  p_name = "设置ab镜头";
}

actn_assets_setname::actn_assets_setname(std::any&& in_any) {
  p_name = "设置名称";
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

}  // namespace doodle
