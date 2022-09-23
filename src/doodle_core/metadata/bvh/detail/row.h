//
// Created by TD on 2022/9/23.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

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

namespace doodle::bvh::detail {
namespace qi    = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
struct DOODLE_CORE_API offset {
  std::double_t x;
  std::double_t y;
  std::double_t z;
  std::vector<std::string> channels_attr;
};

template <typename Iterator>
struct offset_parser_impl : qi::grammar<Iterator, offset(), ascii::space_type> {
  offset_parser_impl() : offset_parser_impl::base_type(start) {
    using ascii::char_;
    using qi::double_;
    using qi::int_;
    using qi::lexeme;
    using qi::lit;
    using qi::no_skip;
    namespace lab = qi::labels;

    text          = lexeme[+(char_ - ' ')[lab::_val += lab::_1]];
    //    end_tag       = int_ >> lit(lab::_r1);
    start %=
        qi::string("OFFSET") >>
        double_ >> double_ >> double_ >>
        qi::string("CHANNELS") >> int_[lab::_r1] >> *text;
  }

  qi::rule<Iterator, offset(), ascii::space_type> start;
  qi::rule<Iterator, std::string(), ascii::space_type> text;
//  qi::rule<Iterator, void(std::string), ascii::space_type> end_tag;
};

using offset_parser = offset_parser_impl<std::string::const_iterator>;

}  // namespace doodle::bvh::detail

// clang-format off
/// @brief 注意此处注册的熟悉顺序

BOOST_FUSION_ADAPT_STRUCT(
    doodle::bvh::detail::offset,
    (std::double_t, x)
    (std::double_t, z)
    (std::double_t, y)
    (std::vector<std::string>, channels_attr)
)

// clang-format onn
