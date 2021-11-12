//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include <doodle_lib/Metadata/metadata.h>
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API assets {
  std::string p_name_show_str;
  FSys::path p_path;

  void set_path_component();

 public:
  std::vector<FSys::path> p_component;
  assets();
  explicit assets(FSys::path in_name);
  // ~Assets();

  [[nodiscard]] std::string str() const;

  void set_path(const FSys::path& in_path);
  const FSys::path& get_path() const;

  [[nodiscard]] std::string show_str() const;

  bool operator<(const assets& in_rhs) const;
  bool operator>(const assets& in_rhs) const;
  bool operator<=(const assets& in_rhs) const;
  bool operator>=(const assets& in_rhs) const;
  bool operator==(const assets& in_rhs) const;
  bool operator!=(const assets& in_rhs) const;
  void attribute_widget(const attribute_factory_ptr& in_factoryPtr);

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version) {
    if (version == 3) {
      ar& BOOST_SERIALIZATION_NVP(p_path);
      ar& BOOST_SERIALIZATION_NVP(p_name_show_str);
      set_path_component();
    }
  };
};
}  // namespace doodle

BOOST_CLASS_VERSION(doodle::assets, 3)
BOOST_CLASS_EXPORT_KEY(doodle::assets)
