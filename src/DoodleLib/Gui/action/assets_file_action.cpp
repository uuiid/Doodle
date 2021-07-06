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

actn_assfile_create::actn_assfile_create(std::any&& in_any)
    {
  p_name = "创建资产文件";
}

actn_assfile_create::actn_assfile_create() {
  p_name = "创建资产文件";
}

void actn_assfile_create::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();
  if (!p_any.has_value()) {
    DOODLE_LOG_WARN("没有发现值")
    return;
  }
  auto k_s = std::any_cast<std::string>(p_any);
  AssetsFilePtr k_item;
  //  if (details::is_class<AssetsFile>(in_data))
  //    k_item = std::make_shared<AssetsFile>(in_data->getParent(), k_s);
  //  else
  k_item = std::make_shared<AssetsFile>(in_data, k_s);
  in_data->child_item.push_back_sig(k_item);

  k_item->updata_db(in_data->getMetadataFactory());
}

actn_assfile_add_com::actn_assfile_add_com(std::any&& in_any) {
  p_name = "添加评论";
}
void actn_assfile_add_com::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
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
actn_assfile_add_com::actn_assfile_add_com() {
  p_name = "添加评论";
}

actn_assfile_datetime::actn_assfile_datetime(std::any&& in_any) {
  p_name = "修改日期";
}
void actn_assfile_datetime::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
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
actn_assfile_datetime::actn_assfile_datetime() {
  p_name = "修改日期";
}

actn_assfile_delete::actn_assfile_delete(std::any&& in_any) {
  p_name = "删除";
}
void actn_assfile_delete::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_ass = std::dynamic_pointer_cast<AssetsFile>(in_data);
  auto k_p   = k_ass->getParent();
  k_p->child_item.erase_sig(k_ass);
  k_ass->deleteData(k_ass->getMetadataFactory());
}
actn_assfile_delete::actn_assfile_delete() {
  p_name = "删除";
}
}  // namespace doodle
