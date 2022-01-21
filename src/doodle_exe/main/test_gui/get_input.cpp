//
// Created by TD on 2022/1/21.
//
#include <doodle_lib/doodle_lib_all.h>

#include <doodle_lib/app/app.h>
#include <doodle_lib/gui/get_input_dialog.h>


using namespace doodle;
int main(int argc, char *argv[])   {
  //初始化测试环境
  boost::locale::generator k_gen{};
  k_gen.categories(boost::locale::all_categories ^
                   boost::locale::formatting_facet ^
                   boost::locale::parsing_facet);
  std::locale::global(k_gen("zh_CN.UTF-8"));
  std::setlocale(LC_NUMERIC, "C");
  std::setlocale(LC_COLLATE, "C");
  std::setlocale(LC_TIME, "C");
  std::setlocale(LC_MONETARY, "C");
  std::setlocale(LC_CTYPE, ".UTF8");
  doodle::app l_app{};
  auto k_h = make_handle();
  k_h.emplace<project>();
//  g_main_loop().attach<get_input_project_dialog>(k_h);
  l_app.run();
  return 0;
}
