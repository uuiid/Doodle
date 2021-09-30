//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include <doodle_lib/Metadata/metadata.h>
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API assets : public metadata {
  std::string p_name;
  std::string p_name_enus;

 public:
  assets();
  explicit assets(std::weak_ptr<metadata> in_metadata, std::string in_name);
  // ~Assets();

  [[nodiscard]] std::string str() const override;
  [[nodiscard]] std::string show_str() const override;

  const std::string& get_name1() const;
  void set_name1(const std::string& in_name);
  const std::string& get_name_enus() const;
  void set_name_enus(const std::string& in_nameEnus);

  bool operator<(const assets& in_rhs) const;
  bool operator>(const assets& in_rhs) const;
  bool operator<=(const assets& in_rhs) const;
  bool operator>=(const assets& in_rhs) const;
  void attribute_widget(const attribute_factory_ptr& in_factoryPtr) override;

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};

template <class Archive>
void assets::serialize(Archive& ar, const std::uint32_t version) {
  if (version == 1)
    ar&
            boost::serialization::make_nvp("metadata", boost::serialization::base_object<metadata>(*this)) &
        p_name;
  if (version == 2)
    ar&
            boost::serialization::make_nvp("metadata", boost::serialization::base_object<metadata>(*this)) &
        p_name&
            p_name_enus;
}
}  // namespace doodle

BOOST_CLASS_VERSION(doodle::assets, 2)
BOOST_CLASS_EXPORT_KEY(doodle::assets)
