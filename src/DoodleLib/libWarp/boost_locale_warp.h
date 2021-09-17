//
// Created by TD on 2021/9/14.
//

#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <boost/locale/util.hpp>
namespace doodle {
namespace conv {
using namespace boost::locale::conv;

template <class CharOut, class CharIn>
std::basic_string<CharOut>
locale_to_utf(std::basic_string<CharIn> const &str, method_type how = default_method) {
  static auto gen = ::boost::locale::generator();
  auto locale_str = ::boost::locale::util::get_system_locale();
  std::vector<std::string> strs{};
  ::boost::split(strs, locale_str, boost::is_any_of("."));
  return to_utf<CharOut>(str, gen(strs.back()), how);
}

template <class CharOut, class CharIn>
std::basic_string<CharOut>
locale_to_utf(const CharIn *begin, method_type how = default_method) {
  static auto gen = ::boost::locale::generator();
  auto locale_str = ::boost::locale::util::get_system_locale();
  std::vector<std::string> strs{};
  ::boost::split(strs, locale_str, boost::is_any_of("."));
  return to_utf<CharOut>(begin, gen(strs.back()), how);
}

template <class CharOut, class CharIn>
std::basic_string<CharOut>
utf_to_locale(std::basic_string<CharIn> const &str, method_type how = default_method) {
  static auto gen = ::boost::locale::generator();
  auto locale_str = ::boost::locale::util::get_system_locale();
  std::vector<std::string> strs{};
  ::boost::split(strs, locale_str, boost::is_any_of("."));
  return from_utf<CharOut>(str, gen(strs.back()), how);
}

}  // namespace conv

}  // namespace doodle
