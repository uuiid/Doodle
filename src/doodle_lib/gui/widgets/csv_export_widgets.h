//
// Created by TD on 2022/2/17.
//

#pragma once
#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_app/gui/base/base_window.h>

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle::gui {

namespace csv_export_widgets_ns {

class csv_line : boost::totally_ordered<csv_line> {
 public:
  csv_line() = default;
  explicit csv_line(
      const entt::handle& in_handle, const entt::handle& in_up_time_handle, const entt::handle& in_user_handle,
      bool in_use_first_as_project_name, const std::string_view& in_season_fmt_str,
      const std::string_view& in_episodes_fmt_str, const std::string_view& in_shot_fmt_str
  );
  /// 按照 季数 -> 集数 -> 镜头 排序
  bool operator<(const csv_line& in_l) const;
  bool operator==(const csv_line& in_l) const;

  /// @brief 部门
  std::string organization_{};
  /// @brief 用户
  std::string user_{};
  /// @brief 项目和季数
  std::string project_season_name_{};
  /// @brief 集数
  std::string episodes_{};
  /// @brief 镜头
  std::string shot_{};
  /// @brief 开始时间
  time_point_wrap start_time_{};
  /// @brief 结束时间
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

class csv_table {
 public:
  csv_table() = default;

  void computing_time();

  void sort_line();

  std::vector<csv_line> line_list;
  std::map<std::string, chrono::seconds> time_statistics;

  std::string to_str() const;
};

}  // namespace csv_export_widgets_ns

class DOODLELIB_API csv_export_widgets : public base_windows<dear::Begin, csv_export_widgets> {
 public:
  using csv_line  = csv_export_widgets_ns::csv_line;
  using csv_table = csv_export_widgets_ns::csv_table;

  enum class work_clock_method : std::uint8_t { form_rule, form_dingding };

 private:
  class impl;
  std::unique_ptr<impl> p_i;
  /**
   * @brief Get the user next time object 获取上一次人物提交时的实体文件
   *
   * @param in
   * @return entt::handle
   */
  entt::handle get_user_up_time(const entt::handle& in_user, const entt::handle& in);

  void generate_table();
  void export_csv();

  void get_work_time();

 public:
  csv_export_widgets();
  ~csv_export_widgets() override;

  constexpr static std::string_view name{gui::config::menu_w::csv_export};

  void init();
  void render();
  const std::string& title() const override;
};

}  // namespace doodle::gui

namespace fmt {
/**
 * 格式化csv line 行
 */
template <>
struct formatter<::doodle::gui::csv_export_widgets_ns::csv_line> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const ::doodle::gui::csv_export_widgets_ns::csv_line& in_, FormatContext& ctx) const
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
