//
// Created by TD on 2021/6/28.
//

#include "assets_file_action.h"

#include <Metadata/Assets.h>
#include <Metadata/AssetsFile.h>
#include <Metadata/AssetsPath.h>
#include <Metadata/Comment.h>
#include <Metadata/TimeDuration.h>
namespace doodle {

assfile_create_action::assfile_create_action(std::any&& in_any)
    : action(std::move(in_any)) {
  p_name = "创建资产文件";
}
void assfile_create_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();
  if (!p_any.has_value()) {
    DOODLE_LOG_WARN("没有发现值")
    return;
  }
  auto k_s    = std::any_cast<std::string>(p_any);
  auto k_item = std::make_shared<Assets>(in_data, k_s);
  in_data->addChildItem(k_item);
  k_item->updata_db(in_data->getMetadataFactory());
}
assfile_create_action::assfile_create_action() {
  p_name = "创建资产文件";
}

assfile_add_com_action::assfile_add_com_action(std::any&& in_any) : action(std::move(in_any)) {
  p_name = "添加评论";
}
void assfile_add_com_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();
  if (!p_any.has_value()) {
    DOODLE_LOG_WARN("没有发现值")
    return;
  }
  auto k_s   = std::any_cast<std::string>(p_any);
  auto k_ass = std::dynamic_pointer_cast<AssetsFile>(in_data);
  k_ass->addComment(std::make_shared<Comment>(k_s));
  k_ass->updata_db(k_ass->getMetadataFactory());
}
assfile_add_com_action::assfile_add_com_action() {
  p_name = "创建资产文件";
}

assfile_datetime_action::assfile_datetime_action(std::any&& in_any) : action(std::move(in_any)) {
  p_name = "修改日期";
}
void assfile_datetime_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();
  if (!p_any.has_value()) {
    DOODLE_LOG_WARN("没有发现值")
    return;
  }
  auto k_time = std::any_cast<TimeDurationPtr>(p_any);
  auto k_ass  = std::dynamic_pointer_cast<AssetsFile>(in_data);
  k_ass->setTime(k_time);
  k_ass->updata_db(k_ass->getMetadataFactory());
}
assfile_datetime_action::assfile_datetime_action() {
  p_name = "修改日期";
}

assfile_delete_action::assfile_delete_action(std::any&& in_any) : action(std::move(in_any)) {
  p_name = "删除";
}
void assfile_delete_action::run(const MetadataPtr& in_data) {
  auto k_ass = std::dynamic_pointer_cast<AssetsFile>(in_data);
  in_data->removeChildItems(k_ass);
  k_ass->deleteData(k_ass->getMetadataFactory());
}
assfile_delete_action::assfile_delete_action() {
  p_name = "删除";
}
}  // namespace doodle
