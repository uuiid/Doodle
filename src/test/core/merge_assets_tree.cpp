//
// Created by TD on 2023/12/7.
//

#include <doodle_core/doodle_core.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/core/app_base.h>


using namespace doodle;

void create_tree(std::int32_t in_index) {
  for (auto i = 0; i < 3; ++i) {
    auto l_handle = entt::handle{*g_reg(), g_reg()->create()};
    l_handle.emplace<database>();
    l_handle.emplace<assets>(fmt::format("test_toot_{}", i));
    for (auto j = 0; j < 5; ++j) {
      auto l_handle_child = entt::handle{*g_reg(), g_reg()->create()};
      l_handle_child.emplace<database>();
      l_handle_child.emplace<assets>(fmt::format("test_toot_{}_{}", i, j));
      l_handle.patch<assets>().add_child(l_handle_child);
      for (auto k = 0; k < 5; ++k) {
        auto l_handle_child_child = entt::handle{*g_reg(), g_reg()->create()};
        l_handle_child_child.emplace<database>();
        l_handle_child_child.emplace<assets>(fmt::format("test_toot_{}_{}_{}", i, j, k));
        l_handle_child.patch<assets>().add_child(l_handle_child_child);
        for (auto l = 0; l < 5; ++l) {
          auto l_handle_ass_file = entt::handle{*g_reg(), g_reg()->create()};
          l_handle_ass_file.emplace<database>();
          l_handle_ass_file.emplace<assets_file>(fmt::format("test_toot_{}_{}_{}_{}_i_{}", i, j, k, l, in_index))
              .assets_attr(l_handle_child);
        }
      }
    }
  };
}

void print_assets() {
  auto l_view = g_reg()->view<assets>().each();
  std::cout << fmt::format("size:{} ", std::distance(l_view.begin(), l_view.end())) << std::endl;

  for (auto&& [e, l_ass] : g_reg()->view<assets>().each()) {
    std::cout << l_ass.p_path << std::endl;
  }
  std::cout << std::endl;
}

int core_merge_assets_tree(int argc, char* argv[]) {
  app_command<> l_app{};

  create_tree(1);
  create_tree(2);
  create_tree(3);
  print_assets();
  assets::merge_assets_tree(g_reg());
  print_assets();
  return 0;
}
