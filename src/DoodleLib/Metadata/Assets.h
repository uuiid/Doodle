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

  [[nodiscard]] std::string str() const override;
  [[nodiscard]] std::string showStr() const override;

  const std::string& getName1() const;
  void setName1(const std::string& in_name);
  const std::string& getNameEnus() const;
  void setNameEnus(const std::string& in_nameEnus);

  void load(const MetadataFactoryPtr& in_factory) override;
  void save(const MetadataFactoryPtr& in_factory) override;

  bool operator<(const Assets& in_rhs) const;
  bool operator>(const Assets& in_rhs) const;
  bool operator<=(const Assets& in_rhs) const;
  bool operator>=(const Assets& in_rhs) const;
  virtual void createMenu(ContextMenu* in_contextMenu) override;
  virtual void deleteData(const MetadataFactoryPtr& in_factory) override;

 protected:
  virtual bool sort(const Metadata& in_rhs) const override;
  void modifyParent(const std::shared_ptr<Metadata> &in_old_parent) override;
  virtual void save() const override;

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
}
}  // namespace doodle

CEREAL_REGISTER_TYPE(doodle::Assets)
CEREAL_CLASS_VERSION(doodle::Assets, 1)
