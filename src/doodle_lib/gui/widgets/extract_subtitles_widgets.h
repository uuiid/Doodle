//
// Created by TD on 2022/4/18.
//
#pragma once

#include <doodle_lib/gui/gui_ref/base_window.h>
namespace doodle::gui {
/**
 * @brief 使用正则表达式提取字幕文本
 * ([^\x00-\xff]+[:|：][^\x00-\xff]+)\n
 *
 */
class DOODLELIB_API extract_subtitles_widgets
    : public window_panel {
  class impl;
  std::unique_ptr<impl> p_i;

  void write_subtitles(const FSys::path& in_soure_file,
                       const FSys::path& out_subtitles_file);

 public:
  extract_subtitles_widgets();
  ~extract_subtitles_widgets() override;
  constexpr static std::string_view name{gui::config::menu_w::long_time_tasks};

 protected:
  void render() override;
};

namespace extract_subtitles_widgets_ns {
constexpr auto init = []() {
  entt::meta<extract_subtitles_widgets>()
      .type()
      .prop("name"_hs, std::string{extract_subtitles_widgets::name})
      .base<gui::window_panel>();
};
class init_class
    : public init_register::registrar_lambda<init, 3> {};
}  // namespace extract_subtitles_widgets_ns
}  // namespace doodle::gui
