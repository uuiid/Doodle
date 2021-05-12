#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/Metadata.h>

namespace doodle {

class DOODLELIB_API Episodes : public Metadata {
  int64_t p_episodes;

 public:
  Episodes();
  explicit Episodes(std::weak_ptr<Metadata> in_metadata, int64_t in_episodes);

  [[nodiscard]] const int64_t &Episodes_() const noexcept;
  void setEpisodes_(const int64_t &Episodes_);

  [[nodiscard]] std::string str() const override;

  virtual void SetPParent(const std::shared_ptr<Metadata> &in_parent) override;
  void load(const MetadataFactoryPtr& in_factory) override;
  void save(const MetadataFactoryPtr& in_factory) override;
 private:
  friend class cereal::access;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);
};
template <class Archive>
void Episodes::serialize(Archive &ar, const std::uint32_t version) {
  if (version == 1)
    ar(cereal::make_nvp("Metadata", cereal::base_class<Metadata>(this)),
       p_episodes);
}
}  // namespace doodle

CEREAL_CLASS_VERSION(doodle::Episodes, 1)