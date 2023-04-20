//
// Created by TD on 2023/1/14.
//
#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/metadata/metadata.h"
#include <doodle_core/doodle_core_fwd.h>

#include <boost/test/unit_test.hpp>

#include "entt/entity/fwd.hpp"
#include <cstdint>
#include <memory>

using namespace doodle;

BOOST_AUTO_TEST_CASE(test_entt_obs) {
  doodle_lib l_lib{};
  entt::observer l_obs{*g_reg(), entt::collector.group<database>()};
  entt::observer l_obs2{*g_reg(), entt::collector.update<database>()};
  auto l_obs3 = std::make_shared<entt::observer>(*g_reg(), entt::collector.group<database>());

  auto l_h    = make_handle();
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