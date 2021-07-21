//
// Created by TD on 2021/7/21.
//

#include "actn_excel.h"

#include <Metadata/Metadata_cpp.h>
#include <core/CoreSet.h>
#include <rpc/RpcMetadataClient.h>

namespace doodle {
actn_export_excel::actn_export_excel()
    : p_list(),
      p_ass_list(),
      p_prj_list(),
      p_user_list() {
  p_name = "导出表格";
  p_term = std::make_shared<long_term>();
}
bool actn_export_excel::is_async() {
  return true;
}
long_term_ptr actn_export_excel::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  _arg_type = sig_get_arg().value();
  if (_arg_type.is_cancel) {
    p_term->sig_finished();
    p_term->sig_message_result({"取消导出"});
  }

  return p_term;
}
void actn_export_excel::export_excel() {
  auto k_rpc = CoreSet::getSet().getRpcMetadataClient();

  auto k_filter = std::make_shared<rpc_filter::filter>();
  k_filter->set_range(_arg_type.p_time_range);
  k_filter->set_meta_type(Metadata::meta_type::file);
  auto k_list = k_rpc->FilterMetadata(k_filter);

  for (const auto& i : k_list) {                            /// 先在每个中循环找到所有的父
    auto k_ptr = std::dynamic_pointer_cast<AssetsFile>(i);  /// 转换为ass file
    if (k_ptr) {                                            /// 确认时ass file 不是其他
      p_ass_list.push_back(k_ptr);                          /// 保存ass file
      find_parent(i);                                       /// 寻找 file 父链
      p_user_list.insert(k_ptr->getUser());                 /// 添加user 为导出user表做准备
    }
  }
  /// 查找所有项目
  std::copy_if(p_list.begin(), p_list.end(), std::inserter(p_prj_list,p_prj_list.begin()),
               [](const std::pair<std::uint64_t, MetadataPtr>& in_) {
                 return !in_.second->hasParent();
               });
}
void actn_export_excel::find_parent(const MetadataPtr& in_ptr) {
  MetadataPtr k_ptr = in_ptr;
  auto k_filter     = std::make_shared<rpc_filter::filter>();
  auto k_rpc        = CoreSet::getSet().getRpcMetadataClient();

  while (k_ptr->hasParent()) {  ///首先测试传入父id 有的话直接查找 p_map 没有就rpc中查找
    const std::uint64_t k_id = k_ptr->get_parent_id();
    if (p_list.find(k_id) == p_list.end()) {
      k_filter->set_id(k_id);
      auto k_item = k_rpc->FilterMetadata(k_filter).front();
      k_filter->reset();
      p_list.insert(std::make_pair(k_item->getId(), k_item));
    }
    p_list[k_id]->child_item.push_back_sig(k_ptr);
    k_ptr = p_list[k_id];
  }
}
void actn_export_excel::export_user_excel() {

}
void actn_export_excel::export_prj_excel() {
}

}  // namespace doodle
