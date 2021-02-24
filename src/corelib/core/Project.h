#pragma once

#include <corelib/core_global.h>
#include <rttr/type>

namespace doodle {

namespace pathParser {
class PathParser;
}

class CORE_API Project {
 private:
  fileSys::path p_root;

  std::vector<std::shared_ptr<fileSys::path>> p_ass_Root;
  std::vector<std::shared_ptr<fileSys::path>> p_shotRoot;

  std::string p_name;
  std::vector<std::shared_ptr<pathParser::PathParser>> p_path_parsers;

  using pathParserList = std::vector<std::shared_ptr<pathParser::PathParser>>;

 public:
  Project(fileSys::path path);
  pathParserList findParser(const rttr::type& type_ptr);

  const std::string& Name() const noexcept;
  void setName(const std::string& Name) noexcept;

  const fileSys::path& Root() const noexcept;
  void setRoot(const fileSys::path& Root) noexcept;

  const std::vector<std::shared_ptr<fileSys::path>>& AssRoot() const noexcept;
  void setAssRoot(const std::vector<std::shared_ptr<fileSys::path>>& AssRoot) noexcept;

  const std::vector<std::shared_ptr<fileSys::path>>& ShotRoot() const noexcept;
  void setShotRoot(const std::vector<std::shared_ptr<fileSys::path>>& ShotRoot) noexcept;
};

}  // namespace doodle