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
}
