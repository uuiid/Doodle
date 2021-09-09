//
// Created by TD on 2021/7/21.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>
#include <DoodleLib/core/CoreSet.h>
namespace doodle {
namespace action_arg {

class DOODLELIB_API arg_excel : public arg_path {
 public:
  using time_point = chrono::sys_time_pos;
  std::pair<TimeDurationPtr, TimeDurationPtr> p_time_range;
};

}  // namespace action_arg

namespace excel {
class DOODLELIB_API assets_file_line : public details::no_copy {
  AssetsFilePtr p_ass_file;

 public:
  explicit assets_file_line(AssetsFilePtr in_ptr);
};
}  // namespace excel

class DOODLELIB_API actn_export_excel : public action_indirect<action_arg::arg_excel> {
  void export_excel();
  void export_dep_excel();
  string_matrix2_ptr export_excel_line(const std::vector<AssetsFilePtr>& in_list);
  void export_user_excel();
  bool exist(const AssetsFilePtr& in_ptr);
  template <class Class>
  std::string get_parent_class_to_str(const AssetsFilePtr& in_assetsFilePtr);


  std::pair<TimeDurationPtr, TimeDurationPtr> find_time(const AssetsFilePtr& in_assetsFilePtr);

  void find_parent(const MetadataPtr& in_ptr);

  std::map<std::uint64_t, MetadataPtr> p_list;
  std::vector<AssetsFilePtr> p_ass_list;
  std::map<std::uint64_t, MetadataPtr> p_prj_list;
  std::set<std::string> p_user_list;
  std::set<Department> p_dep_list;

 public:
  actn_export_excel();
  using arg = action_arg::arg_excel;

  bool is_async() override;
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};

}  // namespace doodle
