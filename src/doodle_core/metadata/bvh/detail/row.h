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
struct DOODLE_CORE_API row {
  std::double_t x;
  std::double_t y;
  std::double_t z;
};

template <typename Iterator>
struct row_parser : qi::grammar<Iterator, row(), ascii::space_type> {
  row_parser() : row_parser::base_type(start) {
    using ascii::char_;
    using qi::double_;
    using qi::int_;
    using qi::lexeme;
    using qi::lit;

    quoted_string %= lexeme['"' >> +(char_ - '"') >> '"'];

    start %=
        lit("employee") >> '{' >> int_ >> ',' >> quoted_string >> ',' >> quoted_string >> ',' >> double_ >> '}';
  }

  qi::rule<Iterator, std::string(), ascii::space_type> quoted_string;
  qi::rule<Iterator, row(), ascii::space_type> start;
};

}  // namespace doodle::bvh::detail

BOOST_FUSION_ADAPT_STRUCT(
    doodle::bvh::detail::row,
    (
        std::double_t, x
    )(
        std::double_t, y
    )(
        std::double_t, z
    )
)
