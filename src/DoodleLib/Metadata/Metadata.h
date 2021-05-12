//
// Created by teXiao on 2021/4/27.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <cereal/types/polymorphic.hpp>
#include <cereal/types/memory.hpp>

namespace doodle {
class DOODLELIB_API Metadata : public std::enable_shared_from_this<Metadata> {
 protected:
  //弱父对象的指针
  std::weak_ptr<Metadata> p_parent;
  //子对象的序列
  std::vector<MetadataPtr> p_child_items;

  //这个时文件的根名称， 基本判断相同就直接比较他俩就行
  std::string p_Root;
  //这个名称保存时的名称（文件名称这个不影响任何判断）
  std::string p_Name;
  //这个时父对象的root
  std::string p_parent_uuid;

  //这个是加载或者保存时的工厂
  //这个工厂会在加载时记录
  //或者在第一次保存时记录
  //基本保证在使用时不空（从逻辑上）
  MetadataFactoryPtr p_metadata_flctory_ptr_;
  virtual bool sort(const Metadata &in_rhs) const = 0;

 public:
  Metadata();
  explicit Metadata(std::weak_ptr<Metadata> in_metadata);
  virtual ~Metadata();
  [[nodiscard]] virtual bool HasParent() const;
  [[nodiscard]] virtual std::shared_ptr<Metadata> GetPParent() const;
  virtual void SetPParent(const std::shared_ptr<Metadata> &in_parent);

  [[nodiscard]] virtual bool HasChild() const;
  [[nodiscard]] virtual const std::vector<MetadataPtr> &GetPChildItems() const;
  virtual bool RemoveChildItems(const MetadataPtr &in_child);
  virtual void SetPChildItems(const std::vector<MetadataPtr> &in_child_items);
  virtual void AddChildItem(const MetadataPtr &in_items);
  virtual void sortChildItems();

  [[nodiscard]] virtual std::string str() const = 0;
  [[nodiscard]] virtual std::string ShowStr() const;

  [[nodiscard]] virtual const std::string &GetRoot() const;
  [[nodiscard]] virtual const std::string &GetRoot();
  [[nodiscard]] virtual const std::string &GetName() const;
  [[nodiscard]] virtual const std::string &GetName();
  //  [[nodiscard]] virtual FSys::path FolderPath() const;

  const MetadataFactoryPtr &GetMetadataFactory() const;

  [[nodiscard]] virtual bool checkParent(const Metadata &in_metadata) const;

  virtual bool operator<(const Metadata &in_rhs) const;
  virtual bool operator>(const Metadata &in_rhs) const;
  virtual bool operator<=(const Metadata &in_rhs) const;
  virtual bool operator>=(const Metadata &in_rhs) const;

  virtual void load(const MetadataFactoryPtr &in_factory);
  virtual void save(const MetadataFactoryPtr &in_factory);
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
CEREAL_REGISTER_TYPE(doodle::Metadata)
// CEREAL_REGISTER_POLYMORPHIC_RELATION(std::enable_shared_from_this<doodle::Metadata>, doodle::Metadata)
CEREAL_CLASS_VERSION(doodle::Metadata, 1)
