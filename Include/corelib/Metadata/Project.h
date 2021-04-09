#pragma once
#include <corelib/core_global.h>

namespace doodle {
class CORE_API Project {
  std::string p_name;
  FSys::path p_path;

 public:
  Project();
  Project(FSys::path in_path, std::string in_name);

  const std::string& Name() const noexcept;
  void setName(const std::string& Name) noexcept;

  const FSys::path& Path() const noexcept;
  void setPath(const FSys::path& Path);
};
}  // namespace doodle