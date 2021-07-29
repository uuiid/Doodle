//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/Metadata.h>

namespace doodle {
class DOODLELIB_API Assets : public Metadata {
  std::string p_name;
  std::string p_name_enus;

 public:
  Assets();
  explicit Assets(std::weak_ptr<Metadata> in_metadata, std::string in_name);
  // ~Assets();

  [[nodiscard]] std::string str() const override;
  [[nodiscard]] std::string showStr() const override;

  const std::string& getName1() const;
  void setName1(const std::string& in_name);
  const std::string& getNameEnus() const;
  void setNameEnus(const std::string& in_nameEnus);

  bool operator<(const Assets& in_rhs) const;
  bool operator>(const Assets& in_rhs) const;
  bool operator<=(const Assets& in_rhs) const;
  bool operator>=(const Assets& in_rhs) const;
  void create_menu(const menu_factory_ptr& in_factoryPtr) override;


 private:
  friend class cereal::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};

template <class Archive>
void Assets::serialize(Archive& ar, const std::uint32_t version) {
  if (version == 1)
    ar(
        cereal::make_nvp("Metadata", cereal::base_class<Metadata>(this)),
        p_name);
  if (version == 2)
    ar(
        cereal::make_nvp("Metadata", cereal::base_class<Metadata>(this)),
        p_name,
        p_name_enus);
}
}  // namespace doodle

CEREAL_REGISTER_TYPE(doodle::Assets)
CEREAL_CLASS_VERSION(doodle::Assets, 2)
