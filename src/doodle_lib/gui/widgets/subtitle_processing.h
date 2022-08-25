//
// Created by TD on 2022/4/21.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>

namespace doodle::gui {
/**
 * @brief 使用正则去除一些东西
 *
 * [^\u4e00-\u9fa5|\(|\)|（|）|：|:]
 */
class DOODLELIB_API subtitle_processing : public window_panel {
  class impl;
  std::unique_ptr<impl> p_i;

  class subtitle_srt_line;

  void run(const FSys::path& in_path, const FSys::path& out_subtitles_file);

 public:
  subtitle_processing();
  ~subtitle_processing() override;
  void init();

  constexpr static std::string_view name{gui::config::menu_w::subtitle_processing};

 protected:
  void render() override;
};

namespace subtitle_processing_ns {
constexpr auto init = []() {
  entt::meta<subtitle_processing>()
      .type()
      .prop("name"_hs, std::string{subtitle_processing::name})
      .base<window_panel>();
};
class init_class
    : public init_register::registrar_lambda<init, 3> {};
}  // namespace subtitle_processing_ns

}  // namespace doodle::gui
