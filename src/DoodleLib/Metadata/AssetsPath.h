//
// Created by TD on 2021/5/18.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
class DOODLELIB_API AssetsPath {
  FSys::path p_path;

 public:
  AssetsPath();
  explicit AssetsPath(const FSys::path &in_path);

  [[nodiscard]] const FSys::path &getPath() const;
  void setPath(const FSys::path &in_path);

//  void open();
 private:
  //这里是序列化的代码
  friend class cereal::access;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);
};

template <class Archive>
void AssetsPath::serialize(Archive &ar, const std::uint32_t version) {
  if(version == 1)
    ar(cereal::make_nvp("path",p_path));
}

}  // namespace doodle

CEREAL_CLASS_VERSION(doodle::AssetsPath,1)
