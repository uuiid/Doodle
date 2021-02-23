#pragma once

#include <corelib/core_global.h>

namespace doodle {

namespace pathParser {
class PathParser;
}

class Project {
 private:
  std::string root;
  std::string name;
  std::vector<std::shared_ptr<pathParser::PathParser>> p_path_parsers;

 public:
  Project();
};

}  // namespace doodle