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

TEST_CASE("type_erasure", "[boost]") {
  namespace mpl = boost::mpl;
  using namespace boost::type_erasure;
  any<mpl::vector<has_push_back<void(int)>, copy_constructible<>, typeid_<>, relaxed>> x{};
  // boost::hana::if_;
  // x = 1;
  x = std::vector<int>{};
  x.push_back(1);
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

void fun(const entt::registry& in_reg,entt::entity in){}



TEST_CASE("entt load", "[boost]") {
  using namespace doodle;
  // auto& set = core_set::getSet();
  entt::registry reg {};

  auto a = reg.create();
  reg.emplace<test_external>(a, 1.f, 1.f);
  auto &k_s = reg.emplace<shot>(a);


  serializeion_warp output;

  reg.destroy(reg.create());

  auto e1 = reg.create();
  auto e3 = reg.create();
  reg.emplace<entt::tag<"empty"_hs>>(e3);

  reg.emplace<test_external>(e1, .8f, .0f);
  entt::snapshot{reg}.entities(output).component<test_external, entt::tag<"empty"_hs>>(output);
}
