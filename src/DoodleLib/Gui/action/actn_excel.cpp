//
// Created by TD on 2021/7/21.
//

#include "actn_excel.h"

#include <Metadata/Metadata_cpp.h>
#include <core/CoreSet.h>
#include <rpc/RpcFileSystemClient.h>
#include <rpc/RpcMetadataClient.h>

#include <csv.hpp>
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
  k_filter->set_range(_arg_type.p_time_range.first->getLocalTime(), _arg_type.p_time_range.second->getLocalTime());
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
  std::copy_if(p_list.begin(), p_list.end(), std::inserter(p_prj_list, p_prj_list.begin()),
               [](const std::pair<std::uint64_t, MetadataPtr>& in_) {
                 return !in_.second->hasParent();
               });
}
void actn_export_excel::find_parent(const MetadataPtr& in_ptr) {
  MetadataPtr k_ptr = in_ptr;
  auto k_filter     = std::make_shared<rpc_filter::filter>();
  auto k_rpc        = CoreSet::getSet().getRpcMetadataClient();

  while (k_ptr->has_parent_id()) {  ///首先测试传入父id 有的话直接查找 p_map 没有就rpc中查找
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
  auto k_path = _arg_type.date;
  if (FSys::exists(k_path))
    FSys::create_directories(k_path);
  std::vector<std::string> k_line{"项目,所属部门,集数,镜头,名称,制作人,开始时间,结束时间,持续时间,备注,文件存在,文件路径"};
  std::vector<AssetsFilePtr> k_list_ass;

  for (const auto& k_user : p_user_list) {
    FSys::ofstream k_ofstream{k_path / (k_user + ".csv"), std::ios::out};
    auto k_csv = csv::make_csv_writer(k_ofstream);
    k_csv << k_line;  ///写入标题

    k_line.clear();
    /// 查找用户所属
    std::copy_if(p_ass_list.begin(), p_ass_list.end(), std::back_inserter(k_list_ass),
                 [k_user](const AssetsFilePtr& in_) {
                   return in_->getUser() == k_user;
                 });
  }
}
void actn_export_excel::export_prj_excel() {
}
string_list_ptr actn_export_excel::export_excel_line(const std::vector<AssetsFilePtr>& in_list) {
  auto k_list = std::make_shared<string_list>();
  //  AssetsFilePtr k_previous;
  TimeDurationPtr k_previous_time = _arg_type.p_time_range.first;
  ;
  for (const auto& k_item : in_list) {
    k_list->emplace_back(k_item->getRootParent()->showStr());              /// 项目
    k_list->emplace_back(magic_enum::enum_name(k_item->getDepartment()));  /// 所属部门
    if (auto k_eps = k_item->find_parent_class<Episodes>(); k_eps) {       ///集数
      k_list->emplace_back(k_eps->str());
    } else {
      k_list->emplace_back(std::string{});
    }
    if (auto k_shot = k_item->find_parent_class<Shot>(); k_shot) {  ///镜头
      k_list->emplace_back(k_shot->str());
    } else
      k_list->emplace_back(std::string{});
    if (auto k_ass = k_item->find_parent_class<Assets>(); k_ass)  ///名称
      k_list->emplace_back(k_ass->str());
    else
      k_list->emplace_back(std::string{});

    k_list->emplace_back(k_item->getUser());  ///制作人

    k_list->emplace_back(k_previous_time->showStr());    ///开始时间
    k_list->emplace_back(k_item->getTime()->showStr());  ///结束时间

    ///持续时间
    const auto& k_com = k_item->getComment();
    k_list->emplace_back(k_com.empty() ? std::string{} : k_com.back()->getComment());   ///备注
    k_list->emplace_back(exist(k_item) ? std::string{"不存在"} : std::string{"存在"});  ///文件存在
    string_list k_string_list;
    std::transform(k_item->getPathFile().begin(), k_item->getPathFile().end(), std::back_inserter(k_string_list),
                   [](const AssetsPathPtr& in_) { return in_->getServerPath().generic_string(); });
    k_list->emplace_back(fmt::format("{}", fmt::join(k_string_list, "\n")));  ///文件路径

    k_previous_time = k_item->getTime();
  }
  return k_list;
}
bool actn_export_excel::exist(const AssetsFilePtr& in_ptr) {
  const auto& k_paths = in_ptr->getPathFile();
  auto k_ch           = CoreSet::getSet().getRpcFileSystemClient();
  if (k_paths.empty())
    return false;
  else
    return std::all_of(k_paths.begin(), k_paths.end(), [k_ch](const AssetsPathPtr& in_) {
      return k_ch->IsExist(in_->getServerPath());
    });
}

}  // namespace doodle
