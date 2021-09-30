//
// Created by TD on 2021/7/29.
//
#include <doodle_lib/doodle_lib_all.h>

#include <catch.hpp>

TEST_CASE("my tree", "[tree]") {
  using namespace doodle;

  auto k_tree = GENERATE(new_object<tree_node>(),
                         new_object<tree_node>(),
                         new_object<tree_node>());
  auto tree   = GENERATE(
        new_object<tree_node>(nullptr, std::make_shared<shot>()),
        new_object<tree_node>(nullptr, std::make_shared<shot>()),
        new_object<tree_node>(nullptr, std::make_shared<episodes>()));
  auto tree2 = GENERATE(
      new_object<tree_node>(nullptr, std::make_shared<shot>()),
      new_object<tree_node>(nullptr, std::make_shared<shot>()),
      new_object<tree_node>(nullptr, std::make_shared<shot>()),
      new_object<tree_node>(nullptr, std::make_shared<episodes>()));

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
        std::cout << it->get()->str() << std::endl;
      }
    }
  }
}
TEST_CASE("date time", "[time]") {
  using namespace doodle;
  date::current_zone();
}
