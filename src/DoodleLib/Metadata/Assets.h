//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/Metadata.h>

namespace doodle {
class DOODLELIB_API Assets : public Metadata {
  std::string p_name;

 public:
  Assets();
  explicit Assets(std::weak_ptr<Metadata> in_metadata, std::string in_name);

  [[nodiscard]] std::string str() const override;
  [[nodiscard]] std::string ShowStr() const override;

 private:
  friend class cereal::access;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);
};

template <class Archive>
void Assets::serialize(Archive &ar, const std::uint32_t version) {
  if (version == 1)
    ar(
        cereal::make_nvp("Metadata", cereal::base_class<Metadata>(this)),
        p_name);
}
}  // namespace doodle

CEREAL_CLASS_VERSION(doodle::Assets,1)