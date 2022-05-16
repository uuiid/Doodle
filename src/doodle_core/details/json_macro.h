//
// Created by TD on 2022/5/16.
//

#pragma once

#include <boost/preprocessor.hpp>

#define DOODLE_JSON_RPC_DETAILS_TO_JSON(r, data, elem) \
  nlohmann_json_j[BOOST_PP_STRINGIZE(elem)] = nlohmann_json_t.elem;

#define DOODLE_JSON_RPC_DETAILS_FORM_JSON(r, data, elem) \
  nlohmann_json_j.at(BOOST_PP_STRINGIZE(elem)).get_to(nlohmann_json_t.elem);

#define DOODLE_JSON_CPP(class_name, ...)                                                                \
  void to_json(nlohmann::json &nlohmann_json_j, const class_name &nlohmann_json_t) {                    \
    BOOST_PP_SEQ_FOR_EACH(DOODLE_JSON_RPC_DETAILS_TO_JSON, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__));   \
  };                                                                                                    \
  void from_json(const nlohmann::json &nlohmann_json_j, class_name &nlohmann_json_t) {                  \
    BOOST_PP_SEQ_FOR_EACH(DOODLE_JSON_RPC_DETAILS_FORM_JSON, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)); \
  };
