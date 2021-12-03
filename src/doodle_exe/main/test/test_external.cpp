//
// Created by TD on 2021/7/29.
//
#include <doodle_lib/doodle_lib_all.h>

#include <catch.hpp>

TEST_CASE("date time", "[time]") {
  using namespace doodle; 
  date::current_zone();
}
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/free.hpp>
#include <boost/type_erasure/member.hpp>
#include <boost/type_erasure/operators.hpp>
BOOST_TYPE_ERASURE_MEMBER(push_back)

class test_to_entt {
  std::int32_t p_t;

 public:
  bool render() {
    using namespace doodle;
    auto k_h = make_handle(*this);
    REQUIRE(k_h);
    return true;
  }
};

TEST_CASE("type_erasure", "[boost]") {
  using namespace doodle;
  auto reg  = g_reg();
  auto k_h  = make_handle(reg->create());
  auto& k_w = k_h.emplace<test_to_entt>(test_to_entt{});
  REQUIRE(to_entity(k_w) == k_h.entity());
  k_w.render();
  std::any k_any;
  entt::enum_as_bitmask<metadata_type>{};
}

struct test_external {
  std::float_t x;
  std::float_t y;
  template <class Archive>
  void serialize(Archive& ar, const std::uint32_t version) {
    ar& BOOST_SERIALIZATION_NVP(x);
    ar& BOOST_SERIALIZATION_NVP(y);
  };
};
BOOST_CLASS_EXPORT(test_external);

class serializeion_warp {
 public:
  serializeion_warp() = default;

  void operator()(entt::entity in) {
    std::cout << 1 << std::endl;
  };
  void operator()(std::underlying_type_t<entt::entity> in) {
    std::cout << 1 << std::endl;
  };
  template <class T>
  void operator()(entt::entity in, const T& t) {
    std::cout << 1 << std::endl;
  };

  void operator()(entt::entity& in) {
    std::cout << 1 << std::endl;
  };
  void operator()(std::underlying_type_t<entt::entity>& in) {
    std::cout << 1 << std::endl;
  };
  template <class T>
  void operator()(entt::entity& in, const T& t) {
    std::cout << 1 << std::endl;
  };
};

void fun(const entt::registry& in_reg, entt::entity in) {}

TEST_CASE("entt load", "[boost]") {
  using namespace doodle;
  // auto& set = core_set::getSet();
  entt::registry reg{};

  auto a = reg.create();
  reg.emplace<test_external>(a, 1.f, 1.f);
  auto& k_s = reg.emplace<shot>(a);

  serializeion_warp output;

  reg.destroy(reg.create());

  auto e1 = reg.create();
  auto e3 = reg.create();
  reg.emplace<entt::tag<"empty"_hs>>(e3);

  reg.emplace<test_external>(e1, .8f, .0f);
  entt::snapshot{reg}.entities(output).component<test_external, entt::tag<"empty"_hs>>(output);
}

template <typename Type>
[[nodiscard]] constexpr auto stripped_type_name() ENTT_NOEXCEPT {
  std::string_view pretty_function{__FUNCSIG__};
  return pretty_function;
}

TEST_CASE("entt stripped_type_name", "[entt]") {
  auto k_l = entt::internal::stripped_type_name<doodle::project>();
  std::cout << k_l << std::endl;
  auto k_l2 = stripped_type_name<doodle::project>();
  std::cout << k_l2 << std::endl;
}
