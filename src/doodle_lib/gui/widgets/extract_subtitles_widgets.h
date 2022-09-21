//
// Created by TD on 2022/4/18.
//
#pragma once

#include <doodle_lib/gui/gui_ref/base_window.h>
namespace doodle::gui {
/**
 * @brief 使用正则表达式提取字幕文本
 * (.+?[:|：].+?)$
 *
 */
class DOODLELIB_API extract_subtitles_widgets
    : public base_windows<dear::Begin, extract_subtitles_widgets> {
  class impl;
  std::unique_ptr<impl> p_i;

  void write_subtitles(const FSys::path& in_soure_file, const FSys::path& out_subtitles_file);

 public:
  extract_subtitles_widgets();
  ~extract_subtitles_widgets() override;

  void init();

  constexpr static std::string_view name{gui::config::menu_w::extract_subtitles};
  const std::string& title() const override;
  void render();
};

}  // namespace doodle::gui
