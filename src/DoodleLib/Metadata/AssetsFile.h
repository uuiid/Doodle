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
};

}  // namespace doodle
