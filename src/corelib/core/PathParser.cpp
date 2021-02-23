#include <corelib/pathParser/PathParser.h>

namespace doodle::pathParser {
PathParser::PathParser(fileSys::path path)
    : p_path_root(std::move(path)),
      p_prefix(),
      p_suffix(),
      p_regex(){

      };

}  // namespace doodle::pathParser