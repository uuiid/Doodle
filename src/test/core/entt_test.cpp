//
// Created by TD on 2023/1/14.
//
#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/metadata/metadata.h"
#include "doodle_core/metadata/project.h"
#include <doodle_core/doodle_core_fwd.h>

#include <boost/test/unit_test.hpp>

#include "entt/entity/fwd.hpp"
#include <cstdint>
#include <memory>

using namespace doodle;
BOOST_AUTO_TEST_SUITE(entt_test)

BOOST_AUTO_TEST_CASE(test_entt_obs) {
  doodle_lib l_lib{};
  entt::observer l_obs{*g_reg(), entt::collector.group<database>()};
  entt::observer l_obs2{*g_reg(), entt::collector.update<database>()};
  auto l_obs3 = std::make_shared<entt::observer>(*g_reg(), entt::collector.group<database>());

  auto l_h    = entt::handle{*g_reg(), g_reg()->create()};
  l_h.emplace<database>();
  l_h.emplace<std::int64_t>();

  BOOST_TEST(l_obs.size() == 1);
  BOOST_TEST(l_obs2.size() == 0);
  BOOST_TEST(l_obs3->size() == 1);

  l_h.emplace_or_replace<database>();
  BOOST_TEST(l_obs.size() == 1);
  BOOST_TEST(l_obs2.size() == 1);
  BOOST_TEST(l_obs3->size() == 1);

  l_h.erase<database>();
  BOOST_TEST(l_obs.size() == 0);
  BOOST_TEST(l_obs2.size() == 0);
  BOOST_TEST(l_obs3->size() == 0);
}

BOOST_AUTO_TEST_CASE(test_entt_obs2) {
  entt::registry reg{};
  entt::observer l_obs1{};
  entt::observer l_obs2{};
  entt::observer l_obs3{};
  l_obs1.connect(reg, entt::collector.group<std::int32_t, std::string>());
  l_obs2.connect(reg, entt::collector.update<std::string>().where<std::int32_t>());
  l_obs3.connect(reg, entt::collector.group<std::int32_t, std::string>().update<std::string>());

  auto l_h = entt::handle{reg, reg.create()};
  l_h.emplace<std::int32_t>();
  l_h.emplace<std::string>();
  auto l_h2 = entt::handle{reg, reg.create()};
  l_h2.emplace<std::int32_t>();
  auto l_h3 = entt::handle{reg, reg.create()};
  l_h3.emplace<std::string>();

  BOOST_TEST(l_obs1.size() == 1);
  BOOST_TEST(l_obs2.size() == 0);
  BOOST_TEST(l_obs3.size() == 1);

  l_h3.emplace_or_replace<std::string>();
  BOOST_TEST(l_obs1.size() == 1);
  BOOST_TEST(l_obs2.size() == 0);
  BOOST_TEST(l_obs3.size() == 2);

  for (auto i : l_obs3) {
    BOOST_TEST_MESSAGE(enum_to_num(i));
  }

  l_h.emplace_or_replace<std::string>();
  BOOST_TEST(l_obs1.size() == 1);
  BOOST_TEST(l_obs2.size() == 1);
  BOOST_TEST(l_obs3.size() == 2);

  l_h.erase<std::int32_t>();
  BOOST_TEST(l_obs1.size() == 0);
  BOOST_TEST(l_obs2.size() == 0);
  BOOST_TEST(l_obs3.size() == 2);
}

class archive_t {
 public:
  void operator()(entt::entity in_entity) { entity_.emplace_back(in_entity); }
  void operator()(std::underlying_type_t<entt::entity> in_underlying_type) {
    underlying_type_.emplace_back(in_underlying_type);
  }
  void operator()(const std::int32_t& in_int_32) { int_32_.emplace_back(in_int_32); }
  void operator()(const std::string& in_string) { string_.emplace_back(in_string); }
  void print() {
    auto l_string = fmt::format(
        "entt [{}] size  [{}] com: [{}] [{}]", fmt::join(entity_, ","), fmt::join(underlying_type_, ","),
        fmt::join(int_32_, ","), fmt::join(string_, ",")

    );
    BOOST_TEST_MESSAGE(l_string);
  }

 private:
  std::vector<std::int32_t> int_32_{};
  std::vector<entt::entity> entity_{};
  std::vector<std::string> string_{};
  std::vector<std::underlying_type_t<entt::entity>> underlying_type_{};
};

BOOST_AUTO_TEST_CASE(save) {
  entt::registry reg{};

  std::vector<entt::entity> l_handles{};
  entt::handle l_h{reg, reg.create()};
  l_h.emplace<std::int32_t>(1);
  l_h.emplace<std::string>("test");
  l_handles.emplace_back(l_h);

  l_h = entt::handle{reg, reg.create()};
  l_h.emplace<std::int32_t>(5);
  l_h.emplace<std::string>("test2");
  l_handles.emplace_back(l_h);

  l_h = entt::handle{reg, reg.create()};
  l_h.emplace<std::string>("test3");

  l_h = entt::handle{reg, reg.create()};
  l_h.emplace<std::int32_t>(5);
  l_h.emplace<std::string>("test2");
  l_h.destroy();

  l_h = entt::handle{reg, reg.create()};
  l_h.emplace<std::int32_t>(5);
  l_h.emplace<std::string>("test2");
  l_h.destroy();

  {
    entt::snapshot l_snapshot{reg};
    archive_t l_archive{};
    BOOST_TEST_MESSAGE("开始保存enttity");
    l_snapshot.get<entt::entity>(l_archive);
    l_archive.print();

    BOOST_TEST_MESSAGE("保存enttity结束,开始保存int32");
    l_snapshot.get<std::int32_t>(l_archive);
    l_archive.print();

    BOOST_TEST_MESSAGE("保存int32结束,开始保存string");
    l_snapshot.get<std::string>(l_archive);
    l_archive.print();

    BOOST_TEST_MESSAGE("保存string结束");
  }

  {
    entt::snapshot l_snapshot{reg};
    archive_t l_archive{};
    BOOST_TEST_MESSAGE("开始保存int32");
    l_snapshot.get<std::int32_t>(l_archive, l_handles.begin(), l_handles.end());
    l_archive.print();

    BOOST_TEST_MESSAGE("保存int32结束,开始保存string");
    l_snapshot.get<std::string>(l_archive, l_handles.begin(), l_handles.end());
    l_archive.print();

    BOOST_TEST_MESSAGE("保存string结束");
  }
}

BOOST_AUTO_TEST_CASE(destroy) {
  entt::registry l_reg{};

  entt::handle l_h1{l_reg, l_reg.create()};
  l_h1.emplace<std::int32_t>(1);
  l_reg.storage<entt::id_type>(12322).emplace(l_h1, 1);
  auto* l_prj = &l_h1.emplace<project>("D:/path", "test");
  l_h1.destroy();
  BOOST_TEST(l_prj == nullptr);


}

BOOST_AUTO_TEST_SUITE_END()