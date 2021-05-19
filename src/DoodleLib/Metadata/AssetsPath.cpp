//
// Created by TD on 2021/5/18.
//

#include <DoodleLib/Metadata/AssetsPath.h>

#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/core/coreset.h>

namespace doodle {
AssetsPath::AssetsPath()
    : p_path() {
}
AssetsPath::AssetsPath(const FSys::path &in_path)
    : p_path() {
  this->setPath(in_path);
}
const FSys::path &AssetsPath::getPath() const {
  return p_path;
}
void AssetsPath::setPath(const FSys::path &in_path) {
  if(!FSys::exists(in_path))
    throw DoodleError{"不存在文件"};
  auto k_path = coreSet::getSet().GetMetadataSet().Project_()->getPath();
  const auto k_root_path = in_path.root_path();
  if (k_root_path == k_path)
    p_path = in_path.lexically_relative(k_path);
  else
    throw DoodleError{"不符合项目根目录"};
}
}  // namespace doodle
