//
// Created by TD on 2021/5/7.
//

#include <DoodleLib/Metadata/AssetsFile.h>

#include <utility>

namespace doodle {

AssetsFile::AssetsFile()
    : Metadata() {
}
AssetsFile::AssetsFile(std::weak_ptr<Metadata> in_metadata)
    : Metadata() {
  p_parent = std::move(in_metadata);
}
std::string AssetsFile::str() const {
  return std::string();
}
std::string AssetsFile::ShowStr() const {
  return Metadata::ShowStr();
}
}  // namespace doodle
