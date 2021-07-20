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

  _assets_file = std::make_shared<AssetsFile>(in_parent, k_s);
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
  k_ass->addComment(std::make_shared<Comment>(k_s));
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
  p_term = std::make_shared<long_term>();
  p_name = "显示详细信息";
}
bool actn_assdile_attr_show::is_async() {
  return true;
}
long_term_ptr actn_assdile_attr_show::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  if (!in_data) {
    p_term->sig_finished();
    p_term->sig_message_result("无法获得数据");
    return p_term;
  }
  auto k_item = std::dynamic_pointer_cast<AssetsFile>(in_data);

  /// TODO: 这里要添加详细信息的面板, 使用信号作为详细信息的显示方案
  sig_get_arg();

  std::string k_com{};
  std::string k_path{};
  for (const auto& k_i : k_item->getComment()) {
    k_com += fmt::format("{}\n", *k_i);
  }
  for (const auto& k_i : k_item->getPathFile()) {
    k_path += fmt::format("{}\n", *k_i);
  }
  auto str = fmt::format(R"(详细信息：
名称： {}
版本： {}
制作者： {}
制作时间： {}
评论：
{}
路径:
{}
)",
                         k_item->showStr(),             //名称
                         k_item->getVersionStr(),       //版本
                         k_item->getUser(),             //制作者
                         k_item->getTime()->showStr(),  //制作时间
                         k_com,                         //评论
                         k_path                         // 路径
  );
}
}  // namespace doodle
