#include <corelib/core/Project.h>
#include <corelib/Exception/Exception.h>
#include <corelib/core/coresql.h>
#include <corelib/core/PathParser.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>

#include <corelib/coreOrm/doodleConfig_sqlOrm.h>
#include <corelib/coreOrm/PathParserData_sqlOrm.h>
#include <corelib/coreOrm/zh_ch_sqlOrm.h>

namespace doodle {
Project::Project(fileSys::path path)
    : p_root(),
      p_ass_Root(),
      p_shotRoot(),
      p_name(),
      p_path_parsers() {
  coreSql::getCoreSql().initDB(path.generic_string());
  auto db = coreSql::getCoreSql().getConnection();

  DoodleConfig table_config{};
  PathParserData table_parser{};

  auto sql = sqlpp::select(table_config.doodleValue)
                 .from(table_config);
  for (auto&& row : (*db)(sql.where(table_config.doodleKey == "Root")))
    p_root = row.doodleValue.value();

  for (auto&& row : (*db)(sql.where(table_config.doodleKey == "Name")))
    p_name = row.doodleValue.value();

  for (auto&& row : (*db)(sql.where(table_config.doodleKey == "assRoot")))
    p_ass_Root.emplace_back(std::make_shared<fileSys::path>(row.doodleValue.value()));

  for (auto&& row : (*db)(sql.where(table_config.doodleKey == "shotRoot")))
    p_shotRoot.emplace_back(std::make_shared<fileSys::path>(row.doodleValue.value()));

  if (p_root != path) throw std::runtime_error("目录无法对应");

  for (auto&& row : (*db)(sqlpp::select(sqlpp::all_of(table_parser))
                              .from(table_parser)
                              .unconditionally())) {
    auto k_parser      = std::make_shared<pathParser::PathParser>();
    k_parser->p_class  = row.refClass.value();
    k_parser->p_prefix = row.prefix.value();
    k_parser->p_regex  = row.regex.value();
    k_parser->p_suffix = row.suffix.value();
    p_path_parsers.emplace_back(k_parser);
  }
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

const fileSys::path& Project::Root() const noexcept {
  return p_root;
}

void Project::setRoot(const fileSys::path& Root) noexcept {
  p_root = Root;
}

const std::vector<std::shared_ptr<fileSys::path>>& Project::AssRoot() const noexcept {
  return p_ass_Root;
}

void Project::setAssRoot(const std::vector<std::shared_ptr<fileSys::path>>& AssRoot) noexcept {
  p_ass_Root = AssRoot;
}

const std::vector<std::shared_ptr<fileSys::path>>& Project::ShotRoot() const noexcept {
  return p_shotRoot;
}

void Project::setShotRoot(const std::vector<std::shared_ptr<fileSys::path>>& ShotRoot) noexcept {
  p_shotRoot = ShotRoot;
}

}  // namespace doodle