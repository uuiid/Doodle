//
// Created by TD on 2021/7/29.
//
#include <DoodleLib/DoodleLib.h>
#include <st_tree.h>

#include <catch.hpp>

TEST_CASE("st tree", "[external]") {
  st_tree::tree<std::string, st_tree::ordered<>> t{};
  t.insert("a");
  t.root().insert("s");
  auto it = t.begin()->find("a");
  if (it != t.root().end())
    std::cout << it->data() << std::endl;

  t.root().find("a");
}
TEST_CASE("my tree", "[tree]") {
  using namespace doodle;
  auto k_tree = std::make_shared<tree_container<std::string>>();
  auto k_p    = k_tree->get_root();
  std::cout << std::boolalpha;
  std::cout << k_tree->has_parent() << std::endl;
  std::cout << k_tree->is_root() << std::endl;
}
