//
// Created by TD on 2021/7/29.
//
#include <DoodleLib/DoodleLib.h>
#include <st_tree.h>

#include <catch.hpp>

TEST_CASE("st tree", "[external]") {
  st_tree::tree<std::string, st_tree::ordered<> > t{};
  t.insert("a");
  t.root().insert("a");
  t.root().find("a");
}
