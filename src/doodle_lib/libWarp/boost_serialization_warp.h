//
// Created by TD on 2021/6/17.
//

#pragma once

//#include <boost/locale.hpp>
#include <date/date.h>

#include <boost/archive/polymorphic_text_iarchive.hpp>
#include <boost/archive/polymorphic_text_oarchive.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/unique_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/weak_ptr.hpp>
//#include <boost/archive/polymorphic_xml_iarchive.hpp>
//#include <boost/archive/polymorphic_xml_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
//#include <boost/archive/xml_iarchive.hpp>
//#include <boost/archive/xml_oarchive.hpp>

#include <optional>
BOOST_SERIALIZATION_SPLIT_FREE(std::filesystem::path)

// BOOST_CLASS_EXPORT_KEY(std::filesystem::path)
//
//
// namespace boost {
// namespace serialization {
// template <class T>
// struct guid_defined<std::optional<T>> : boost::mpl::true_ {};
// template <class T>
// inline const char* guid<std::optional<T>>() { return "std::optional<T>"; }
// }  // namespace serialization
// }  // namespace boos

namespace boost::serialization {

template <class Archive>
inline void save(Archive &ar, const std::filesystem::path &path, const std::uint32_t version) {
  ar &boost::serialization::make_nvp("path", path.generic_string());
}
template <class Archive>
inline void load(Archive &ar, std::filesystem::path &path, const std::uint32_t version) {
  std::string str{};
  ar &boost::serialization::make_nvp("path", str);
  path = std::filesystem::path{str};
};

template <class Archive, class T>
inline void save(
    Archive &ar,
    const std::optional<T> &t,
    const unsigned int /*version*/
) {
  auto has_val = t.has_value();
  if (!has_val) {
    ar &boost::serialization::make_nvp("has_val", has_val);
  } else {
    ar &boost::serialization::make_nvp("has_val", has_val);
    ar &boost::serialization::make_nvp("data", *t);
  }
};

template <class Archive, class T>
inline void load(
    Archive &ar,
    std::optional<T> &t,
    const unsigned int version) {
  bool has_val;
  ar &boost::serialization::make_nvp("has_val", has_val);
  if (has_val) {
    T val{};
    ar &boost::serialization::make_nvp("data", val);
    t = std::move(val);
  } else {
    t = std::nullopt;
  }
};

template <class Archive, class T>
inline void serialize(
    Archive &ar,
    std::optional<T> &t,
    const unsigned int version) {
  boost::serialization::split_free(ar, t, version);
}

// template <class Archive>
// std::int32_t save_minimal(Archive const &ar, date::year const &in_year) {
//   return (int)in_year;
// }
// template <class Archive>
// void load_minimal(Archive const &, date::year &in_year, std::int32_t const &value) {
//   in_year = date::year{value};
// }
//
// template <class Archive>
// std::uint32_t save_minimal(Archive const &ar, date::month const &in_month) {
//   return (std::uint32_t)in_month;
// }
// template <class Archive>
// void load_minimal(Archive const &, date::month &in_month, std::uint32_t const &value) {
//   in_month = date::month{value};
// }
//
// template <class Archive>
// std::uint32_t save_minimal(Archive const &ar, date::day const &in_day) {
//   return (std::uint32_t)in_day;
// }
// template <class Archive>
// void load_minimal(Archive const &, date::day &in_day, std::uint32_t const &value) {
//   in_day = date::day{value};
// }
}  // namespace boost::serialization
