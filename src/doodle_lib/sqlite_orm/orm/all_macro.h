#pragma once

#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include <tuple>

#define DOODLE_SELECT_VALUE_TUPLE_ELEM_(r, data, i, elem) BOOST_PP_COMMA_IF(i) BOOST_PP_TUPLE_ELEM(2, 0, elem)

#define DOODLE_SELECT_VALUE_FIELD_DECL_(r, data, elem) \
  decltype(BOOST_PP_TUPLE_ELEM(2, 0, elem)) BOOST_PP_TUPLE_ELEM(2, 1, elem);

#define DOODLE_SELECT_VALUE(struct_name, ...)                                                              \
  struct struct_name {                                                                                     \
    constexpr static auto select_value = std::make_tuple(                                                  \
        BOOST_PP_SEQ_FOR_EACH_I(DOODLE_SELECT_VALUE_TUPLE_ELEM_, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
    );                                                                                                     \
    BOOST_PP_SEQ_FOR_EACH(DOODLE_SELECT_VALUE_FIELD_DECL_, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))       \
  };
