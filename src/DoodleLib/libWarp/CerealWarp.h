//
// Created by TD on 2021/6/17.
//

#pragma once

#include <cereal/access.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
namespace cereal {

template <class Archive>
inline void save(Archive &ar, boost::filesystem::path const &path) {
  ar(cereal::make_nvp("path", path.generic_string()));
}
template <class Archive>
inline void load(Archive &ar, boost::filesystem::path &path) {
  std::string str{};
  ar(str);
  path = boost::filesystem::path{str};
};

template <class Archive>
std::int32_t save_minimal(Archive const &ar, date::year const &in_year) {
  return (int)in_year;
}
template <class Archive>
void load_minimal(Archive const &, date::year &in_year, std::int32_t const &value) {
  in_year = date::year{value};
}

template <class Archive>
std::uint32_t save_minimal(Archive const &ar, date::month const &in_month) {
  return (std::uint32_t)in_month;
}
template <class Archive>
void load_minimal(Archive const &, date::month &in_month, std::uint32_t const &value) {
  in_month = date::month{value};
}

template <class Archive>
std::uint32_t save_minimal(Archive const &ar, date::day const &in_day) {
  return (std::uint32_t)in_day;
}
template <class Archive>
void load_minimal(Archive const &, date::day &in_day, std::uint32_t const &value) {
  in_day = date::day{value};
}
}  // namespace cereal
