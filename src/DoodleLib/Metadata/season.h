//
// Created by TD on 2021/7/29.
//

#pragma once
#include <DoodleLib/Metadata/Metadata.h>
#include <DoodleLib_fwd.h>
namespace doodle {

class DOODLELIB_API season
    : public Metadata {
  std::int32_t p_int;

 public:
  season();

  void set_season(std::int32_t in_);
  std::int32_t get_season(std::int32_t in_) const;

  explicit season(std::weak_ptr<Metadata> in_metadata, std::int32_t in_);
  virtual std::string str() const override;
  virtual void create_menu(const menu_factory_ptr& in_factoryPtr) override;

 private:
  friend class cereal::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};

template <class Archive>
void season::serialize(Archive& ar, std::uint32_t const version) {
  if (version == 1)
    ar(
        cereal::make_nvp("Metadata", cereal::base_class<Metadata>(this)),
        p_int);
}
}  // namespace doodle

CEREAL_REGISTER_TYPE(doodle::season)
CEREAL_CLASS_VERSION(doodle::season, 1)
