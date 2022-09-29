//
// Created by TD on 2022/2/17.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_app/gui/base/base_window.h>
namespace doodle::gui {

class DOODLELIB_API csv_export_widgets
    : public base_windows<dear::Begin, csv_export_widgets> {
 public:
  class csv_line {
   public:
    std::string organization_;
    std::string user_;
    std::string season_name_;
    std::string episodes_;
    std::string shot_;
    std::string start_time_;
    std::string end_time_;
    std::string len_time_;
    std::string time_info_;
    std::string comment_info_;
    std::string file_path_;
    std::string name_attr_;
    std::string cutoff_attr_;
  };

 private:
  class impl;
  std::unique_ptr<impl> p_i;
  /**
   * @brief Get the user next time object 获取上一次人物提交时的实体文件
   *
   * @param in
   * @return entt::handle
   */
  time_point_wrap get_user_up_time(const entt::handle& in_user, const entt::handle& in);
  /**
   * @brief 导出单行使用的函数
   *
   * @param in 传入的一行实体
   * @return table_line 返回的一行
   */
  csv_line to_csv_line(const entt::handle& in_user, const entt::handle& in);
  /**
   * @brief 导出单张表使用的函数
   *
   * @param in_list
   */
  void export_csv(const std::vector<entt::handle>& in_list, const FSys::path& in_export_file_path);

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
 * @brief 集数格式化程序
 *
 * @tparam
 */
template <>
struct formatter<::doodle::gui::csv_export_widgets::csv_line> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const ::doodle::gui::csv_export_widgets::csv_line& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
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
        in_.organization_,
        in_.user_,
        in_.season_name_,
        in_.episodes_,
        in_.shot_,
        in_.start_time_,
        in_.end_time_,
        in_.len_time_,
        in_.time_info_,
        in_.comment_info_,
        in_.file_path_,
        in_.name_attr_,
        in_.cutoff_attr_
    );
    return ctx.out();
  }
};
}  // namespace fmt
