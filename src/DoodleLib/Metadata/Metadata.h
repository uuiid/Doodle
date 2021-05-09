//
// Created by teXiao on 2021/4/27.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <cereal/types/polymorphic.hpp>
namespace doodle {
class DOODLELIB_API Metadata {
 protected:
  std::weak_ptr<Metadata> p_parent;
  std::vector<MetadataPtr> p_child_items;

  std::string p_Root;
  std::string p_Name;
  std::string p_parent_uuid;

 public:
  Metadata();
  virtual ~Metadata();
  [[nodiscard]] bool HasParent() const;
  [[nodiscard]] std::shared_ptr<Metadata> GetPParent() const;
  void SetPParent(const std::shared_ptr<Metadata> &in_parent);

  [[nodiscard]] bool HasChild() const;
  [[nodiscard]] const std::vector<MetadataPtr> &GetPChildItems() const;
  void SetPChildItems(const std::vector<MetadataPtr> &in_child_items);
  void AddChildItem(const MetadataPtr &in_items);

  [[nodiscard]] virtual std::string str() const = 0;
  [[nodiscard]] virtual std::string ShowStr() const;

  [[nodiscard]] virtual const std::string &GetRoot() const;
  [[nodiscard]] virtual const std::string &GetRoot();
  [[nodiscard]] virtual const std::string &GetName() const;
  [[nodiscard]] virtual const std::string &GetName();
  //  [[nodiscard]] virtual FSys::path FolderPath() const;

  [[nodiscard]] virtual bool checkParent(const Metadata& in_metadata) const;

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);
};

template <class Archive>
void Metadata::serialize(Archive &ar, std::uint32_t const version) {
  if (version == 1)
    ar(
        cereal::make_nvp("UUID_Root", p_Root),
        cereal::make_nvp("UUID_name", p_Name),
        cereal::make_nvp("UUID_parent", p_parent_uuid));
}
}  // namespace doodle
CEREAL_CLASS_VERSION(doodle::Metadata, 1)
