//
// Created by TD on 2022/9/23.
//
#define BOOST_TEST_MODULE doodle lib
#define BOOST_SPIRIT_X3_DEBUG
#define BOOST_SPIRIT_DEBUG

#include <cstdlib>
#include <crtdbg.h>

#include <iostream>
#include <iomanip>
#include <iostream>

#include <boost/test/included/unit_test.hpp>
#include <doodle_core/doodle_core.h>
// #include <doodle_core/metadata/bvh/detail/row.h>

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

#include <doodle_core/doodle_core_fwd.h>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/support.hpp>

// #include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/lex.hpp>
#include <boost/spirit/include/phoenix1.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support.hpp>
#include <boost/spirit/repository/include/qi_kwd.hpp>
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
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <utility>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
// #include <boost/>

class bvh_data {
 public:
  const static std::string bvh_data_attr;
  const static std::string bvh_data_attr_node;
};

namespace doodle::bvh::detail {
namespace qi    = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

struct node;

using rational_int = boost::rational<std::size_t>;
// using rational_int = std::float_t;

struct node {
  std::string name;
  std::double_t x;
  std::double_t y;
  std::double_t z;
  std::vector<std::string> channels_attr;
  std::vector<node> childs;
};

struct bvh_tree {
  bvh_tree() = default;

  explicit bvh_tree(
      std::size_t in_frames,
      rational_int in_frame_time,
      node in_root
  ) : frames(in_frames),
      frame_time(in_frame_time),
      root(std::move(in_root)) {}

  std::size_t frames;
  rational_int frame_time;
  node root;
};
using boost::fusion::operator<<;

}  // namespace doodle::bvh::detail

// clang-format off
/// @brief 注意此处注册的熟悉顺序

BOOST_FUSION_ADAPT_STRUCT(
    doodle::bvh::detail::node,
    (std::string, name)
    (std::double_t, x)
    (std::double_t, y)
    (std::double_t, z)
    (std::vector<std::string>, channels_attr)
    (std::vector<doodle::bvh::detail::node>, childs)
)
BOOST_FUSION_ADAPT_STRUCT(
    doodle::bvh::detail::bvh_tree,
    (std::size_t, frames)
    (doodle::bvh::detail::rational_int, frame_time)
    (doodle::bvh::detail::node, root)
)
// clang-format on

namespace fmt {
template <>
struct formatter<doodle::bvh::detail::bvh_tree> : formatter<std::int64_t> {
  template <typename FormatContext>
  auto format(const doodle::bvh::detail::bvh_tree& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return formatter<std::int64_t>::format(
        in_.frames,
        ctx
    );
  }
};
template <>
struct formatter<doodle::bvh::detail::node> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const doodle::bvh::detail::node& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return formatter<std::string>::format(
        in_.name,
        ctx
    );
  }
};
}  // namespace fmt

namespace doodle::bvh::detail {

struct print_rul {
  template <typename T>
  void operator()(T const& i, qi::unused_type, qi::unused_type) const {
    BOOST_TEST_MESSAGE(fmt::to_string(i));
  }
};

template <typename Iterator>
struct bvh_node_impl : qi::grammar<Iterator, node(), ascii::space_type> {
  bvh_node_impl() : bvh_node_impl::base_type(start) {
    using ascii::char_;
    using boost::spirit::repository::qi::kwd;
    using qi::double_;
    using qi::int_;
    using qi::lexeme;
    using qi::lit;
    using qi::no_skip;

    namespace phoe = boost::phoenix;
    namespace lab  = qi::labels;

    join_name %= lexeme[+qi::alnum];

    start = qi::string("HIERARCHY") >>
            (qi::string("ROOT") | qi::string("JOINT") | qi::string("End")) >>
            join_name[phoe::at_c<0>(qi::_val) = qi::_1] >>

            qi::char_('{') >>
            qi::string("OFFSET") >>
            double_[phoe::at_c<1>(qi::_val) = qi::_1] >>
            double_[phoe::at_c<2>(qi::_val) = qi::_1] >>
            double_[phoe::at_c<3>(qi::_val) = qi::_1] >>
            qi::string("CHANNELS") >> int_ >>
            *join_name[phoe::push_back(phoe::at_c<4>(qi::_val), qi::_1)] >>

            qi::char_('}');

    BOOST_SPIRIT_DEBUG_NODES((start));
  }

  qi::rule<Iterator, node(), ascii::space_type> start;
  qi::rule<Iterator, node(), ascii::space_type> node_rule;
  qi::rule<Iterator, std::string(), ascii::space_type> join_name;
};

template <typename Iterator>
struct bvh_tree_impl : qi::grammar<Iterator, bvh_tree(), ascii::space_type> {
  bvh_tree_impl() : bvh_tree_impl::base_type(start) {
    namespace phoe = boost::phoenix;
    namespace lab  = qi::labels;

    name_str %= qi::lexeme[+qi::alnum];

    end_node = qi::string("End") >>
               name_str[phoe::at_c<0>(qi::_val) = qi::_1] >>
               qi::char_('{') >>
               qi::string("OFFSET") >>
               qi::double_[phoe::at_c<1>(qi::_val) = qi::_1] >>
               qi::double_[phoe::at_c<2>(qi::_val) = qi::_1] >>
               qi::double_[phoe::at_c<3>(qi::_val) = qi::_1] >>
               qi::char_('}');
    int n;
    node_rule =
        (qi::string("ROOT") | qi::string("JOINT")) >>
        name_str[phoe::at_c<0>(qi::_val) = qi::_1] >>

        qi::char_('{') >>
        qi::string("OFFSET") >>
        qi::double_[phoe::at_c<1>(qi::_val) = qi::_1] >>
        qi::double_[phoe::at_c<2>(qi::_val) = qi::_1] >>
        qi::double_[phoe::at_c<3>(qi::_val) = qi::_1] >>
        qi::string("CHANNELS") >> qi::int_[phoe::ref(n) = qi::_1] >>
        qi::repeat(phoe::ref(n))[name_str[print_rul{}]] >>
        *(node_rule | end_node)[print_rul{}] >>
        qi::char_('}');

    start = qi::string("HIERARCHY") >>
            node_rule[print_rul{}] >>
            //            node_rule[phoe::at_c<2>(qi::_val) = qi::_1] >>
            qi::string("MOTION") >>
            qi::string("Frames:") >> qi::int_[phoe::at_c<0>(qi::_val) = qi::_1] >>
            qi::string("Frame") >> qi::string("Time:") >>
            qi::float_[phoe::at_c<1>(qi::_val) = rational_int{}] >>
            *qi::float_;  // std::float_t(qi::_1 * 1000), 1000.0f

    BOOST_SPIRIT_DEBUG_NODE(start);
  }

  qi::rule<Iterator, bvh_tree(), ascii::space_type> start;
  qi::rule<Iterator, node(), ascii::space_type> node_rule;
  qi::rule<Iterator, node(), ascii::space_type> end_node;
  qi::rule<Iterator, std::string(), ascii::space_type> name_str;

  //  qi::rule<Iterator, void(std::string), ascii::space_type> end_tag;
};

using bvh_tree_parse = bvh_tree_impl<std::string::const_iterator>;
using bvh_node_parse = bvh_node_impl<std::string::const_iterator>;
}  // namespace doodle::bvh::detail

struct loop_fixtures {
  loop_fixtures()  = default;
  ~loop_fixtures() = default;

  // std::shared_ptr<doodle_lib> l_lib{};
  void setup() {
    // l_lib = std::make_shared<doodle_lib>();
    // doodle_lib::create_time_database();
    BOOST_TEST_MESSAGE("完成夹具设置");
  };
  void teardown(){
      // l_lib.reset();
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

BOOST_AUTO_TEST_CASE(bvh_mpl) {
  doodle::bvh::detail::bvh_tree l_w{
      100ull,
      doodle::bvh::detail::rational_int{60},
      doodle::bvh::detail::node{}};
  auto l_str = boost::fusion::as_vector(l_w);

  BOOST_TEST_MESSAGE(boost::fusion::at_c<0>(l_str));
  // BOOST_TEST_MESSAGE(boost::rational_cast<std::double_t>(boost::fusion::at_c<1>(l_str)));
  // BOOST_TEST_MESSAGE(boost::fusion::at_c<2>(l_str));
}

BOOST_AUTO_TEST_CASE(bvh_node, *boost::unit_test::tolerance(0.00001)) {
  namespace qi    = boost::spirit::qi;
  namespace ascii = boost::spirit::ascii;

  doodle::bvh::detail::bvh_node_parse l_p{};
  doodle::bvh::detail::node l_w{};
  auto l_r = boost::spirit::qi::phrase_parse(
      bvh_data::bvh_data_attr_node.begin(), bvh_data::bvh_data_attr_node.end(), l_p, ascii::space, l_w
  );

  BOOST_TEST(l_r);
  BOOST_TEST_MESSAGE(l_w.name);
  BOOST_TEST_MESSAGE(l_w.x);
  BOOST_TEST_MESSAGE(l_w.y);
  BOOST_TEST_MESSAGE(l_w.z);
  //  boost::fusion::for_each(boost::fusion::as_vector(l_w), std::cout << boost::mpl::_1);
  // BOOST_TEST(boost::rational_cast<std::double_t>(l_w.frame_time) == 0.000);
}

BOOST_AUTO_TEST_CASE(bvh_tree, *boost::unit_test::tolerance(0.00001)) {
  namespace qi    = boost::spirit::qi;
  namespace ascii = boost::spirit::ascii;

  doodle::bvh::detail::bvh_tree_parse l_p{};
  doodle::bvh::detail::bvh_tree l_w{};
  auto l_r = boost::spirit::qi::phrase_parse(
      bvh_data::bvh_data_attr.begin(), bvh_data::bvh_data_attr.end(), l_p, ascii::space, l_w
  );

  BOOST_TEST(l_r);
  BOOST_TEST_MESSAGE(l_w.frames);

  //  boost::fusion::for_each(boost::fusion::as_vector(l_w), std::cout << boost::mpl::_1);
  // BOOST_TEST(boost::rational_cast<std::double_t>(l_w.frame_time) == 0.000);
}

BOOST_AUTO_TEST_SUITE_END()

const std::string bvh_data::bvh_data_attr      = R"(
HIERARCHY
ROOT Hips
{
	OFFSET	0.00	0.00	0.00
	CHANNELS 6 Xposition Yposition Zposition Zrotation Xrotation Yrotation
	JOINT Chest
	{
		OFFSET	 0.00	 5.21	 0.00
		CHANNELS 3 Zrotation Xrotation Yrotation
		JOINT Neck
		{
			OFFSET	 0.00	 18.65	 0.00
                      CHANNELS 3 Zrotation Xrotation Yrotation
			JOINT Head
			{
				OFFSET	 0.00	 5.45	 0.00
				CHANNELS 3 Zrotation Xrotation Yrotation
				End Site
				{
					OFFSET	 0.00	 3.87	 0.00
				}
			}
		}
		JOINT LeftCollar
		{
			OFFSET	 1.12	 16.23	 1.87
			CHANNELS 3 Zrotation Xrotation Yrotation
			JOINT LeftUpArm
			{
				OFFSET	 5.54	 0.00	 0.00
				CHANNELS 3 Zrotation Xrotation Yrotation
				JOINT LeftLowArm
				{
					OFFSET	 0.00	-11.96	 0.00
					CHANNELS 3 Zrotation Xrotation Yrotation
					JOINT LeftHand
					{
						OFFSET	 0.00	-9.93	 0.00
						CHANNELS 3 Zrotation Xrotation Yrotation
						End Site
						{
							OFFSET	 0.00	-7.00	 0.00
						}
					}
				}
			}
		}
		JOINT RightCollar
		{
			OFFSET	-1.12	 16.23	 1.87
			CHANNELS 3 Zrotation Xrotation Yrotation
			JOINT RightUpArm
			{
				OFFSET	-6.07	 0.00	 0.00
				CHANNELS 3 Zrotation Xrotation Yrotation
				JOINT RightLowArm
				{
					OFFSET	 0.00	-11.82	 0.00
					CHANNELS 3 Zrotation Xrotation Yrotation
					JOINT RightHand
					{
						OFFSET	 0.00	-10.65	 0.00
						CHANNELS 3 Zrotation Xrotation Yrotation
						End Site
						{
							OFFSET	 0.00	-7.00	 0.00
						}
					}
				}
			}
		}
	}
	JOINT LeftUpLeg
	{
		OFFSET	 3.91	 0.00	 0.00
		CHANNELS 3 Zrotation Xrotation Yrotation
		JOINT LeftLowLeg
		{
			OFFSET	 0.00	-18.34	 0.00
			CHANNELS 3 Zrotation Xrotation Yrotation
			JOINT LeftFoot
			{
				OFFSET	 0.00	-17.37	 0.00
				CHANNELS 3 Zrotation Xrotation Yrotation
				End Site
				{
					OFFSET	 0.00	-3.46	 0.00
				}
			}
		}
	}
	JOINT RightUpLeg
	{
		OFFSET	-3.91	 0.00	 0.00
		CHANNELS 3 Zrotation Xrotation Yrotation
		JOINT RightLowLeg
		{
			OFFSET	 0.00	-17.63	 0.00
			CHANNELS 3 Zrotation Xrotation Yrotation
			JOINT RightFoot
			{
				OFFSET	 0.00	-17.14	 0.00
				CHANNELS 3 Zrotation Xrotation Yrotation
				End Site
				{
					OFFSET	 0.00	-3.75	 0.00
				}
			}
		}
	}
}
MOTION
Frames:    2
Frame Time: 0.033333
 8.03	 35.01	 88.36	-3.41	 14.78	-164.35	 13.09	 40.30	-24.60	 7.88	 43.80	 0.00	-3.61	-41.45	 5.82	 10.08	 0.00	 10.21	 97.95	-23.53	-2.14	-101.86	-80.77	-98.91	 0.69	 0.03	 0.00	-14.04	 0.00	-10.50	-85.52	-13.72	-102.93	 61.91	-61.18	 65.18	-1.57	 0.69	 0.02	 15.00	 22.78	-5.92	 14.93	 49.99	 6.60	 0.00	-1.14	 0.00	-16.58	-10.51	-3.11	 15.38	 52.66	-21.80	 0.00	-23.95	 0.00
 7.81	 35.10	 86.47	-3.78	 12.94	-166.97	 12.64	 42.57	-22.34	 7.67	 43.61	 0.00	-4.23	-41.41	 4.89	 19.10	 0.00	 4.16	 93.12	-9.69	-9.43	 132.67	-81.86	 136.80	 0.70	 0.37	 0.00	-8.62	 0.00	-21.82	-87.31	-27.57	-100.09	 56.17	-61.56	 58.72	-1.63	 0.95	 0.03	 13.16	 15.44	-3.56	 7.97	 59.29	 4.97	 0.00	 1.64	 0.00	-17.18	-10.02	-3.08	 13.56	 53.38	-18.07	 0.00	-25.93	 0.00
)";

const std::string bvh_data::bvh_data_attr_node = R"(HIERARCHY
ROOT Hips
{
OFFSET	1.00	2.00	3.00
CHANNELS 6 Xposition Yposition Zposition Zrotation Xrotation Yrotation
}
)";
