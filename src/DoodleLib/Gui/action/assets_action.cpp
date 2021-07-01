//
// Created by TD on 2021/6/28.
//

#include "assets_action.h"

#include <Metadata/Assets.h>
#include <Metadata/Episodes.h>
#include <Metadata/Shot.h>
namespace doodle {

assset_create_action::assset_create_action(std::any&& in_any)
    : action(std::move(in_any)) {
  p_name = "创建类别";
}
void assset_create_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();

  if (!p_any.has_value()) {
    DOODLE_LOG_WARN("没有发现值")
    return;
  }

  auto k_s = std::any_cast<std::string>(p_any);
  if (k_s.empty()) {
    DOODLE_LOG_WARN("没有值")
    return;
  }
  auto k_a = std::make_shared<Assets>(in_data, k_s);
  in_data->addChildItem(k_a);
  k_a->updata_db(in_data->getMetadataFactory());
}
assset_create_action::assset_create_action() {
  p_name = "创建类别";
}

episode_create_action::episode_create_action(std::any&& in_any)
    : action(std::move(in_any)) {
  p_name = "创建集数";
}
void episode_create_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();

  if (!p_any.has_value()) {
    DOODLE_LOG_WARN("没有发现值")
    return;
  }

  auto k_i    = std::any_cast<std::int32_t>(p_any);
  auto k_item = std::make_shared<Episodes>(in_data, k_i);
  in_data->addChildItem(k_item);
  k_item->updata_db(in_data->getMetadataFactory());
}
episode_create_action::episode_create_action() {
  p_name = "创建集数";
}

shot_create_action::shot_create_action(std::any&& in_any)
    : action(std::move(in_any)) {
  p_name = "创建镜头";
}
shot_create_action::shot_create_action() {
  p_name = "创建镜头";
}

void shot_create_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();

  if (!p_any.has_value()) {
    DOODLE_LOG_WARN("没有发现值")
    return;
  }

  auto k_i    = std::any_cast<std::int32_t>(p_any);
  auto k_item = std::make_shared<Shot>(in_data, k_i);
  in_data->addChildItem(k_item);
  k_item->updata_db(in_data->getMetadataFactory());
}

assets_delete_action::assets_delete_action(std::any&& in_any) : action(std::move(in_any)) {
  p_name = "删除";
}
void assets_delete_action::run(const MetadataPtr& in_data) {
  in_data->deleteData(in_data->getMetadataFactory());
  auto k_p = in_data->getParent();
  k_p->removeChildItems(in_data);
}
assets_delete_action::assets_delete_action() {
  p_name = "删除";
}

episode_set_action::episode_set_action(std::any&& in_any) : action(std::move(in_any)) {
  p_name = "设置集数";
}
void episode_set_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();

  if (!p_any.has_value()) {
    DOODLE_LOG_WARN("没有发现值")
    return;
  }

  auto k_i   = std::any_cast<std::int32_t>(p_any);
  auto k_eps = std::dynamic_pointer_cast<Episodes>(in_data);
  k_eps->setEpisodes(k_i);
  k_eps->updata_db(k_eps->getMetadataFactory());
}
episode_set_action::episode_set_action() {
  p_name = "设置集数";
}

shot_set_action::shot_set_action(std::any&& in_any) : action(std::move(in_any)) {
  p_name = "设置镜头号";
}
void shot_set_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();

  if (!p_any.has_value()) {
    DOODLE_LOG_WARN("没有发现值")
    return;
  }

  auto k_i    = std::any_cast<std::int32_t>(p_any);
  auto k_item = std::dynamic_pointer_cast<Shot>(in_data);
  k_item->setShot(k_i);
  k_item->updata_db(k_item->getMetadataFactory());
}
shot_set_action::shot_set_action() {
  p_name = "设置镜头号";
}

shotab_set_action::shotab_set_action(std::any&& in_any) : action(std::move(in_any)) {
  p_name = "设置ab镜头";
}
void shotab_set_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();

  if (!p_any.has_value()) {
    DOODLE_LOG_WARN("没有发现值")
    return;
  }

  auto k_s    = std::any_cast<Shot::ShotAbEnum>(p_any);
  auto k_item = std::dynamic_pointer_cast<Shot>(in_data);
  k_item->setShotAb(k_s);
  k_item->updata_db(k_item->getMetadataFactory());
}
shotab_set_action::shotab_set_action() {
  p_name = "设置ab镜头";
}

assets_setname_action::assets_setname_action(std::any&& in_any) : action(std::move(in_any)) {
  p_name = "设置名称";
}
void assets_setname_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();

  if (!p_any.has_value()) {
    DOODLE_LOG_WARN("没有发现值")
    return;
  }

  auto k_s    = std::any_cast<std::string>(p_any);
  auto k_item = std::dynamic_pointer_cast<Assets>(in_data);
  k_item->setName1(k_s);
  k_item->updata_db(k_item->getMetadataFactory());
}
assets_setname_action::assets_setname_action() {
  p_name = "设置名称";
}

}  // namespace doodle
