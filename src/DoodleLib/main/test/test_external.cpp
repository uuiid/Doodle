//
// Created by TD on 2021/7/29.
//
#include <DoodleLib/DoodleLib.h>

#include <catch.hpp>

TEST_CASE("my tree", "[tree]") {
  using namespace doodle;

  auto k_tree = GENERATE(make_tree(), make_tree(), make_tree());
  auto tree   = GENERATE_COPY(
      make_tree(nullptr, std::make_shared<Shot>()),
      make_tree(k_tree, std::make_shared<Shot>()),
      make_tree(k_tree, std::make_shared<Episodes>()));
  auto tree2 = GENERATE_COPY(
      make_tree(nullptr, std::make_shared<Shot>()),
      make_tree(k_tree, std::make_shared<Shot>()),
      make_tree(tree, std::make_shared<Shot>()),
      make_tree(tree, std::make_shared<Episodes>()));

  SECTION("tree insert") {
    k_tree->insert(tree);

    k_tree->insert(tree2);
    SECTION("tree mut insert") {
      k_tree->insert(tree);
    }
    SECTION("remove tree") {
      k_tree->erase(tree);
    }
    SECTION("clear item") {
      k_tree->clear();
    }
    SECTION("for child") {
      for (auto& it : k_tree->get_children()) {
        if (it.get())
          std::cout << it.get()->str() << std::endl;
      }
    }
  }
}
TEST_CASE("date time", "[time]") {
  using namespace doodle;
  date::current_zone();
}
