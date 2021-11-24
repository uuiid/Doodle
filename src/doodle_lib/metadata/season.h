//
// Created by TD on 2021/7/29.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/metadata/metadata.h>
namespace doodle {

class DOODLELIB_API season {
  std::int32_t p_int;

 public:
  season();
  explicit season( std::int32_t in_);

  void set_season(std::int32_t in_);
  std::int32_t get_season() const;

  virtual std::string str() const ;
  virtual void attribute_widget(const attribute_factory_ptr& in_factoryPtr) ;
  bool operator<(const season& in_rhs) const;
  bool operator>(const season& in_rhs) const;
  bool operator<=(const season& in_rhs) const;
  bool operator>=(const season& in_rhs) const;
  bool operator==(const season& in_rhs) const;
  bool operator!=(const season& in_rhs) const;

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};

template <class Archive>
void season::serialize(Archive& ar, std::uint32_t const version) {
  if (version == 1)
    ar& BOOST_SERIALIZATION_NVP(p_int);
}
}  // namespace doodle

BOOST_CLASS_VERSION(doodle::season, 1)
BOOST_CLASS_EXPORT_KEY(doodle::season)
