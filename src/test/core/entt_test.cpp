//
// Created by TD on 2023/1/14.
//
#include <doodle_core/doodle_core_fwd.h>

#include <boost/test/unit_test.hpp>

using namespace doodle;

BOOST_AUTO_TEST_CASE(test_entt_obs) {
  entt::registry reg{};
  entt::observer l_obs{};
  entt::observer l_obs2{};
  l_obs.connect(reg, entt::collector.group<std::int32_t>());
  l_obs2.connect(reg, entt::collector.update<std::int32_t>());

  auto l_h = entt::handle{reg, reg.create()};
  l_h.emplace<std::int32_t>();

  BOOST_TEST(l_obs.size() == 1);
  BOOST_TEST(l_obs2.size() == 0);

  l_h.emplace_or_replace<std::int32_t>();
  BOOST_TEST(l_obs.size() == 1);
  BOOST_TEST(l_obs2.size() == 1);

  l_h.erase<std::int32_t>();
  BOOST_TEST(l_obs.size() == 0);
  BOOST_TEST(l_obs2.size() == 0);
}

BOOST_AUTO_TEST_CASE(test_entt_obs2) {
  entt::registry reg{};
  entt::observer l_obs{};
  entt::observer l_obs2{};
  l_obs.connect(reg, entt::collector.group<std::int32_t, std::string>());
  l_obs2.connect(reg, entt::collector.update<std::string>().where<std::int32_t>());

  auto l_h = entt::handle{reg, reg.create()};
  l_h.emplace<std::int32_t>();
  l_h.emplace<std::string>();
  auto l_h2 = entt::handle{reg, reg.create()};
  l_h2.emplace<std::int32_t>();
  auto l_h3 = entt::handle{reg, reg.create()};
  l_h3.emplace<std::string>();

  BOOST_TEST(l_obs.size() == 1);
  BOOST_TEST(l_obs2.size() == 0);

  l_h3.emplace_or_replace<std::string>();
  l_h.emplace_or_replace<std::string>();
  BOOST_TEST(l_obs.size() == 1);
  BOOST_TEST(l_obs2.size() == 1);

  l_h.erase<std::int32_t>();
  BOOST_TEST(l_obs.size() == 0);
  BOOST_TEST(l_obs2.size() == 0);
}