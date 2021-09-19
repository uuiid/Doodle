//
// Created by TD on 2021/7/21.
//

#include "actn_excel.h"

#include <DoodleLib/Metadata/Metadata_cpp.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/rpc/RpcFileSystemClient.h>
#include <DoodleLib/rpc/RpcMetadataClient.h>
#include <DoodleLib/core/DoodleLib.h>

#include <csv.hpp>
namespace doodle {
actn_export_excel::actn_export_excel()
    : p_list(),
      p_ass_list(),
      p_prj_list(),
      p_user_list() {
  p_name = "导出表格";
}
bool actn_export_excel::is_async() {
  return true;
}
long_term_ptr actn_export_excel::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  _arg_type = sig_get_arg().value();
  auto k_term = get_long_term_signal();
  if (_arg_type.is_cancel || !_arg_type.p_time_range.first || !_arg_type.p_time_range.second) {
    k_term->sig_finished();
    k_term->sig_message_result({"取消导出\n"},long_term::warning);
  }
  export_excel();
  return k_term;
}
void actn_export_excel::export_excel() {
  auto k_rpc = DoodleLib::Get().getRpcMetadataClient();

  auto k_filter = new_object<rpc_filter::filter>();
  DOODLE_LOG_INFO("获得日期 {} {}", _arg_type.p_time_range.first->showStr(), _arg_type.p_time_range.second->showStr());
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
  /// 先按照时间排序
  std::sort(p_ass_list.begin(), p_ass_list.end(), [](const AssetsFilePtr& in_l, const AssetsFilePtr& in_r) {
    return in_l->getTime()->getLocalTime() < in_r->getTime()->getLocalTime();
  });
  /// 查找所有项目
  std::copy_if(p_list.begin(), p_list.end(), std::inserter(p_prj_list, p_prj_list.begin()),
               [](const std::pair<std::uint64_t, MetadataPtr>& in_) {
                 return !in_.second->hasParent();
               });
  std::transform(p_ass_list.begin(), p_ass_list.end(),
                 std::inserter(p_dep_list, p_dep_list.begin()),
                 [](const AssetsFilePtr& in_) {
                   return in_->getDepartment();
                 });
  export_user_excel();
  export_dep_excel();
}

void actn_export_excel::export_user_excel() {
  auto k_path = _arg_type.date;
  if (!FSys::exists(k_path))
    FSys::create_directories(k_path);
  std::vector<std::string> k_line{"项目", "所属部门", "集数", "镜头", "名称", "制作人", "开始时间", "结束时间", "持续时间", "备注", "文件存在", "文件路径"};

  for (const auto& k_user : p_user_list) {
    std::vector<AssetsFilePtr> k_list_ass;
    FSys::ofstream k_ofstream{k_path / (k_user + ".csv"), std::ios::out};
    auto k_csv = csv::make_csv_writer(k_ofstream);
    k_csv << k_line;  ///写入标题
    /// 查找用户所属
    std::copy_if(p_ass_list.begin(), p_ass_list.end(), std::back_inserter(k_list_ass),
                 [k_user](const AssetsFilePtr& in_) {
                   return in_->getUser() == k_user;
                 });
    auto k_mat = export_excel_line(k_list_ass);
    /// 再进行集数排序
    std::stable_sort(k_mat->begin(), k_mat->end(), [](const string_list& in_l, const string_list& in_r) {
      return in_l[2] < in_r[2];
    });

    /// 先进行项目排序
    std::stable_sort(k_mat->begin(), k_mat->end(), [](const string_list& in_l, const string_list& in_r) {
      return in_l[0] < in_r[0];
    });

    for (auto& k_l : *k_mat)
      k_csv << k_l;
  }
  DOODLE_LOG_INFO("完成导出 user 表");
}

void actn_export_excel::export_dep_excel() {
  auto k_path = _arg_type.date;
  if (!FSys::exists(k_path))
    FSys::create_directories(k_path);
  std::vector<std::string> k_line{"项目", "所属部门", "集数", "镜头", "名称", "制作人", "开始时间", "结束时间", "持续时间", "备注", "文件存在", "文件路径"};

  for (const auto& k_dep : p_dep_list) {
    std::vector<AssetsFilePtr> k_list_ass;
    FSys::ofstream k_ofstream{k_path / (std::string{magic_enum::enum_name(k_dep)} + ".csv"), std::ios::out};
    auto k_csv = csv::make_csv_writer(k_ofstream);
    k_csv << k_line;  ///写入标题
    /// 查找用户所属
    std::copy_if(p_ass_list.begin(), p_ass_list.end(), std::back_inserter(k_list_ass),
                 [k_dep](const AssetsFilePtr& in_) {
                   return in_->getDepartment() == k_dep;
                 });
    auto k_mat = export_excel_line(k_list_ass);
    /// 再进行集数排序
    std::stable_sort(k_mat->begin(), k_mat->end(), [](const string_list& in_l, const string_list& in_r) {
      return in_l[5] < in_r[5];
    });

    /// 再进行集数排序
    std::stable_sort(k_mat->begin(), k_mat->end(), [](const string_list& in_l, const string_list& in_r) {
      return in_l[2] < in_r[2];
    });

    /// 先进行项目排序
    std::stable_sort(k_mat->begin(), k_mat->end(), [](const string_list& in_l, const string_list& in_r) {
      return in_l[0] < in_r[0];
    });

    for (auto& k_l : *k_mat)
      k_csv << k_l;
  }
  DOODLE_LOG_INFO("完成导出 Department 表");
}

void actn_export_excel::find_parent(const MetadataPtr& in_ptr) {
  MetadataPtr k_ptr = in_ptr;
  auto k_filter     = new_object<rpc_filter::filter>();
  auto k_rpc        = DoodleLib::Get().getRpcMetadataClient();

  while (k_ptr->has_parent_id() && k_ptr->get_parent_id() != 0) {  ///首先测试传入父id 有的话直接查找 p_map 没有就rpc中查找
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

string_matrix2_ptr actn_export_excel::export_excel_line(const std::vector<AssetsFilePtr>& in_list) {
  auto k_matrix2 = new_object<string_matrix2>();

  for (const auto& k_item : in_list) {
    auto k_list = string_list{};
    k_list.emplace_back(fmt::format("《{}》", k_item->getRootParent()->showStr()));  /// 项目
    k_list.emplace_back(magic_enum::enum_name(k_item->getDepartment()));             /// 所属部门

    k_list.emplace_back(k_item->find_parent_class_to_string<Episodes>());  ///集数
    k_list.emplace_back(k_item->find_parent_class_to_string<Shot>());      ///镜头
    k_list.emplace_back(k_item->find_parent_class_to_string<Assets>());    ///名称

    k_list.emplace_back(k_item->getUser());  ///制作人

    auto k_time = find_time(k_item);
    k_list.emplace_back(fmt::format("\"{}\"", k_time.first->showStr()));   ///开始时间
    k_list.emplace_back(fmt::format("\"{}\"", k_time.second->showStr()));  ///结束时间

    auto k_work_time = k_time.first->work_duration(*k_time.second);
    k_list.emplace_back(std::to_string(chrono::floor<chrono::days_double>(k_work_time).count()));  ///持续时间
    const auto& k_com = k_item->getComment();
    k_list.emplace_back(k_com.empty() ? std::string{"无"} : k_com.back()->getComment());  ///备注
    k_list.emplace_back(exist(k_item) ? std::string{"存在"} : std::string{"不存在"});     ///文件存在
    string_list k_string_list;
    std::transform(k_item->getPathFile().begin(), k_item->getPathFile().end(), std::back_inserter(k_string_list),
                   [](const AssetsPathPtr& in_) { return in_->getServerPath().generic_string(); });
    k_list.emplace_back(fmt::format("{}", fmt::join(k_string_list, "\n")));  ///文件路径

    k_matrix2->emplace_back(k_list);
  }
  return k_matrix2;
}

bool actn_export_excel::exist(const AssetsFilePtr& in_ptr) {
  const auto& k_paths = in_ptr->getPathFile();
  auto k_ch           = DoodleLib::Get().getRpcFileSystemClient();
  if (k_paths.empty())
    return false;
  else
    return std::all_of(k_paths.begin(), k_paths.end(), [k_ch](const AssetsPathPtr& in_) {
      return k_ch->IsExist(in_->getServerPath());
    });
}
std::pair<TimeDurationPtr, TimeDurationPtr> actn_export_excel::find_time(const AssetsFilePtr& in_assetsFilePtr) {
  std::pair<TimeDurationPtr, TimeDurationPtr> k_pair{};
  k_pair.first = in_assetsFilePtr->getTime();

  auto k_it = std::find(p_ass_list.begin(), p_ass_list.end(), in_assetsFilePtr);
  if (k_it == p_ass_list.end())
    throw DoodleError{"错误的类型"};
  ++k_it;

  if (k_it == p_ass_list.end()) {
    k_pair.second = _arg_type.p_time_range.second;
  } else {
    auto k_it_next = std::find_if(k_it, p_ass_list.end(), [in_assetsFilePtr](const AssetsFilePtr& in_ptr) {
      return in_ptr->getUser() == in_assetsFilePtr->getUser();
    });
    if (k_it_next == p_ass_list.end())
      k_pair.second = _arg_type.p_time_range.second;
    else
      k_pair.second = (*k_it_next)->getTime();
  }
  return k_pair;
}

}  // namespace doodle
