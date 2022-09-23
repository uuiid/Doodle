//
// Created by TD on 2022/9/23.
//

 
#include <cstdlib>
#include <crtdbg.h>

 

#define BOOST_TEST_MODULE doodle lib

#include <boost/test/included/unit_test.hpp>
#include <doodle_core/doodle_core.h>
#include <doodle_core/metadata/bvh/detail/row.h>

// #include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/lex.hpp>
#include <boost/spirit/include/phoenix1.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support.hpp>
// #include <boost/spirit/home/classic.hpp>
#include <boost/spirit/home/karma.hpp>
#include <boost/spirit/home/lex.hpp>
#include <boost/spirit/home/qi.hpp>
#include <boost/spirit/home/support.hpp>
#include <boost/spirit/version.hpp>
#include <boost/phoenix.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/stl.hpp>

using namespace doodle;

struct loop_fixtures {
  loop_fixtures()  = default;
  ~loop_fixtures() = default;

  std::shared_ptr<doodle_lib> l_lib{};
  void setup() {
    l_lib = std::make_shared<doodle_lib>();
    doodle_lib::create_time_database();
    BOOST_TEST_MESSAGE("完成夹具设置");
  };
  void teardown() {
    l_lib.reset();
  };
};

BOOST_FIXTURE_TEST_SUITE(spirit_test, loop_fixtures)

BOOST_AUTO_TEST_CASE(test_base_spirit) {
  namespace qi    = boost::spirit::qi;
  namespace ascii = boost::spirit::ascii;
  using ascii::space;
  using boost::phoenix::push_back;
  using qi::_1;
  using qi::double_;
  using qi::int_;
  using qi::parse;
  using qi::phrase_parse;
  std::string l_p_str{"1,2,3,4,5"};
  std::vector<std::double_t> l_r{1, 2, 3, 4, 5};

  {
    std::vector<std::double_t> l_num;
    auto l_p = (double_[push_back(boost::phoenix::ref(l_num), _1)] >> *(',' >> double_[push_back(boost::phoenix::ref(l_num), _1)]));
    boost::spirit::qi::phrase_parse(l_p_str.begin(), l_p_str.end(), l_p, space);
    BOOST_TEST((l_num == l_r));
  }

  {
    std::vector<std::double_t> l_num;
    auto l_p2 = double_[push_back(boost::phoenix::ref(l_num), _1)] % ',';
    boost::spirit::qi::phrase_parse(l_p_str.begin(), l_p_str.end(), l_p2, space);
    BOOST_TEST((l_num == l_r));
  }

  {
    std::vector<std::double_t> l_num;
    auto l_p2 = double_ % ',';
    boost::spirit::qi::phrase_parse(l_p_str.begin(), l_p_str.end(), l_p2, space, l_num);
    BOOST_TEST((l_num == l_r));
  }
}

BOOST_AUTO_TEST_CASE(bvh_offest, *boost::unit_test::tolerance(0.00001)) {
  std::string l_data{"OFFSET 0.000 91.545 0.000"};
  namespace qi    = boost::spirit::qi;
  namespace ascii = boost::spirit::ascii;

  doodle::bvh::detail::offset_parser l_p{};
  doodle::bvh::detail::offset l_w{};
  auto l_r = boost::spirit::qi::phrase_parse(
      l_data.begin(), l_data.end(), l_p, ascii::space, l_w
  );

  BOOST_TEST(l_r);
  BOOST_TEST(l_w.x == 0.000);
  BOOST_TEST(l_w.y == 91.545);
  BOOST_TEST(l_w.z == 0.000);
}

BOOST_AUTO_TEST_SUITE_END()
