//
// Created by TD on 2022/2/17.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>
namespace doodle::gui {

class DOODLELIB_API csv_export_widgets
    : public window_panel {
  class impl;
  std::unique_ptr<impl> p_i;
  using table_line = std::array<std::string, 13>;
  /**
   * @brief Get the user next time object 获取上一次人物提交时的实体文件
   *
   * @param in
   * @return entt::handle
   */
  time_point_wrap get_user_up_time(const entt::handle& in);
  /**
   * @brief 导出单行使用的函数
   *
   * @param in 传入的一行实体
   * @return table_line 返回的一行
   */
  table_line to_csv_line(const entt::handle& in);
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
  void failed();
  void render() override;
};
namespace csv_export_widgets_ns {
constexpr auto init = []() {
  entt::meta<csv_export_widgets>()
      .type()
      .prop("name"_hs, std::string{csv_export_widgets::name})
      .base<gui::window_panel>();
};
class [[maybe_unused]] init_class
    : public init_register::registrar_lambda<init, 3> {};
}  // namespace csv_export_widgets_ns
}  // namespace doodle::gui
