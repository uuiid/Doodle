//
// Created by TD on 2022/2/17.
//

#pragma once
#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_app/gui/base/base_window.h>

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle::gui {

namespace xlsx_export_widgets_ns {

class xlsx_line : boost::totally_ordered<xlsx_line> {
 public:
  xlsx_line() = default;
  explicit xlsx_line(
      const entt::handle& in_handle, const std::vector<entt::handle>& in_up_time_handle_list,
      const entt::handle& in_user_handle, bool in_use_first_as_project_name, const std::string_view& in_season_fmt_str,
      const std::string_view& in_episodes_fmt_str, const std::string_view& in_shot_fmt_str
  );
  /// 按照 季数 -> 集数 -> 镜头 排序
  bool operator<(const xlsx_line& in_l) const;
  bool operator==(const xlsx_line& in_l) const;

  /// 部门
  std::string organization_{};
  /// 用户
  std::string user_{};
  /// 项目和季数
  std::string project_season_name_{};
  /// 集数
  std::string episodes_{};
  /// 镜头
  std::string shot_{};
  /// 开始时间
  time_point_wrap start_time_{};
  /// 结束时间
  time_point_wrap end_time_{};
  /// 时间长度
  chrono::seconds len_time_{};
  /// 时间信息
  std::string time_info_{};
  /// 提交信息
  std::string comment_info_{};
  /// 路径
  std::string file_path_{};
  /// 名称
  std::string name_attr_{};
  /// 等级
  std::string cutoff_attr_{};

 private:
  template <typename T1, typename Char, typename Enable>
  friend struct fmt::formatter;
};

class xlsx_table {
 public:
  xlsx_table() = default;

  void computing_time();

  void sort_line();

  std::vector<xlsx_line> line_list;
  std::map<std::string, chrono::seconds> time_statistics;

  std::string to_str() const;
};

}  // namespace xlsx_export_widgets_ns

class DOODLELIB_API xlsx_export_widgets {
 public:
  using xlsx_line  = xlsx_export_widgets_ns::xlsx_line;
  using xlsx_table = xlsx_export_widgets_ns::xlsx_table;

  enum class work_clock_method : std::uint8_t { form_rule, form_dingding };

 private:
  class impl;
  std::unique_ptr<impl> p_i;

  void generate_table();
  /// 导出表
  void export_xlsx();

  bool get_work_time();
  void gen_user();

  void filter_();

 public:
  xlsx_export_widgets();
  ~xlsx_export_widgets();

  constexpr static std::string_view name{gui::config::menu_w::xlsx_export};

  void init();
  bool render();
  const std::string& title() const;
};

}  // namespace doodle::gui

namespace fmt {
/**
 * 格式化xlsx line 行
 */
template <>
struct formatter<::doodle::gui::xlsx_export_widgets_ns::xlsx_line> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const ::doodle::gui::xlsx_export_widgets_ns::xlsx_line& in_, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    fmt::format_to(
        ctx.out(),
        "{},"
        "{},"
        "{},"
        "{},"
        "{},"
        "{},"
        "{},"
        "{},"
        "{},"
        "{},"
        "{},"
        "{},"
        "{}",
        in_.organization_, in_.user_, in_.project_season_name_, in_.episodes_, in_.shot_, in_.start_time_,
        in_.end_time_, in_.len_time_, in_.time_info_, in_.comment_info_, in_.file_path_, in_.name_attr_,
        in_.cutoff_attr_
    );
    return ctx.out();
  }
};
}  // namespace fmt
