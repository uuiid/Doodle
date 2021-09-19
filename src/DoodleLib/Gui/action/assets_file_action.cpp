//
// Created by TD on 2021/6/28.
//

#include "assets_file_action.h"

#include <DoodleLib/Metadata/Assets.h>
#include <DoodleLib/Metadata/AssetsFile.h>
#include <DoodleLib/Metadata/AssetsPath.h>
#include <DoodleLib/Metadata/Comment.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <DoodleLib/Metadata/time_point_wrap.h>
namespace doodle {

actn_assfile_create::actn_assfile_create()
    : _assets_file() {
  p_name = "创建资产文件";
}

AssetsFilePtr actn_assfile_create::get_result() {
  return _assets_file;
}

long_term_ptr actn_assfile_create::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_s = sig_get_arg().value().date;
  AssetsFilePtr k_item;

  _assets_file = new_object<AssetsFile>(in_parent, k_s);
  in_parent->child_item.push_back_sig(_assets_file);
  _assets_file->setVersion(_assets_file->find_max_version());
  _assets_file->updata_db(in_parent->getMetadataFactory());
  in_parent->sortChildItems(true);
  return {};
}

actn_assfile_add_com::actn_assfile_add_com(std::any&& in_any) {
  p_name = "添加评论";
}
long_term_ptr actn_assfile_add_com::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_s   = sig_get_arg().value().date;
  auto k_ass = std::dynamic_pointer_cast<AssetsFile>(in_data);
  k_ass->addComment(new_object<Comment>(k_s));
  k_ass->updata_db(k_ass->getMetadataFactory());
  return {};
}
actn_assfile_add_com::actn_assfile_add_com() {
  p_name = "添加评论";
}

actn_assfile_datetime::actn_assfile_datetime(std::any&& in_any) {
  p_name = "修改日期";
}
long_term_ptr actn_assfile_datetime::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_time = sig_get_arg().value().time;
  auto k_ass  = std::dynamic_pointer_cast<AssetsFile>(in_data);
  k_ass->setTime(k_time);
  k_ass->updata_db(k_ass->getMetadataFactory());
  return {};
}
actn_assfile_datetime::actn_assfile_datetime() {
  p_name = "修改日期";
}
long_term_ptr actn_assfile_delete::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_ass = std::dynamic_pointer_cast<AssetsFile>(in_data);
  auto k_p   = k_ass->getParent();
  try {
    k_p->child_item.erase_sig(k_ass);
    k_ass->deleteData(k_ass->getMetadataFactory());
  } catch (const std::runtime_error&) {
    DOODLE_LOG_WARN("无法找到 id {} {} ", k_ass->getId(), k_ass->showStr())
  }
  return {};
}
actn_assfile_delete::actn_assfile_delete() {
  p_name = "删除";
}
actn_assdile_attr_show::actn_assdile_attr_show() {
  p_name = "显示详细信息";
}
bool actn_assdile_attr_show::is_async() {
  return true;
}
long_term_ptr actn_assdile_attr_show::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_term = this->get_long_term_signal();
  if (!in_data) {
    this->cancel("无法获得数据");
    return k_term;
  }

  sig_get_arg();
  k_term->sig_finished();
  k_term->sig_message_result({},long_term::warning);
  return k_term;
}
}  // namespace doodle
