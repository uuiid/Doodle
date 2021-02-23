#pragma once

#include <corelib/core_global.h>
#include <rttr/type>

namespace doodle {

namespace pathParser {
class PathParser;
}

class Project {
 private:
  fileSys::path p_root;
  std::string p_name;
  std::vector<std::shared_ptr<pathParser::PathParser>> p_path_parsers;

  using pathParserList = std::vector<std::shared_ptr<pathParser::PathParser>>;

 public:
  Project(fileSys::path path);
  pathParserList findParser(const rttr::type& type_ptr);

  const std::string& Name() const noexcept;
  void setName(const std::string& Name) noexcept;

};

}  // namespace doodle