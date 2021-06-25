//
// Created by TD on 2021/6/24.
//

#include "main_windows.h"
namespace doodle {

main_windows::main_windows()
    : nana::form(nana::API::make_center(500, 600)),
      p_layout(*this),
      p_menubar(*this),
      p_menu() {
  p_layout.div(
      R"(<vertical <weight=23 menubar> >)");
  auto& k_menu = p_menubar.push_back("文件");
  k_menu.append("打开", [](nana::menu::item_proxy &) {nana::API::exit_all();});

  p_layout.field("menubar") << p_menubar;

  p_layout.collocate();
}
}  // namespace doodle
