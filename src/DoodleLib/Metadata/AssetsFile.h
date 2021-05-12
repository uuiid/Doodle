//
// Created by TD on 2021/5/7.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/Metadata.h>

namespace doodle {
class AssetsFile : public Metadata {
  std::string p_name;
  std::string p_ShowName;
  FSys::path p_path_file;

 public:
  AssetsFile();
  explicit AssetsFile(std::weak_ptr<Metadata> in_metadata, std::string name = {}, std::string showName = {});
  [[nodiscard]] std::string str() const override;
  [[nodiscard]] std::string ShowStr() const override;
  
  virtual void SetPParent(const std::shared_ptr<Metadata> &in_parent) override;
  void load(const MetadataFactoryPtr& in_factory) override;
  void save(const MetadataFactoryPtr& in_factory) override;

 private:
  friend class cereal::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};

template <class Archive>
void AssetsFile::serialize(Archive& ar, const std::uint32_t version) {
  if (version == 1)
    ar(
        cereal::make_nvp("Metadata", cereal::base_class<Metadata>(this)),
        p_name,
        p_ShowName,
        p_path_file);
}

}  // namespace doodle
