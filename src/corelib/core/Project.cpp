#include <corelib/core/Project.h>
#include <corelib/core/PathParser.h>
namespace doodle {
Project::Project(fileSys::path path)
    : p_root(),
      p_name(),
      p_path_parsers() {
}

Project::pathParserList Project::findParser(const rttr::type& type_ptr) {
  pathParserList k_parser_list{};
  for (auto&& p : p_path_parsers) {
    if (p->p_class == type_ptr.get_name()) {
      k_parser_list.push_back(p);
    }
  }
  return k_parser_list;
}

const std::string& Project::Name() const noexcept {
  return p_name;
}

void Project::setName(const std::string& Name) noexcept {
  p_name = Name;
}

}  // namespace doodle