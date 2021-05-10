//
// Created by TD on 2021/5/7.
//

#include <DoodleLib/Metadata/AssetsFile.h>
#include <PinYin/convert.h>
#include <utility>

namespace doodle {

AssetsFile::AssetsFile()
    : Metadata() {
}
AssetsFile::AssetsFile(std::weak_ptr<Metadata> in_metadata, std::string name, std::string showName)
    : Metadata(),
      p_name(std::move(name)),
      p_ShowName(std::move(showName)) {
  p_parent = std::move(in_metadata);
  if(p_ShowName.empty())
    p_ShowName = convert::Get().toEn(p_name);
}

std::string AssetsFile::str() const {
  return p_name;
}
std::string AssetsFile::ShowStr() const {
  return p_ShowName;
}
}  // namespace doodle
