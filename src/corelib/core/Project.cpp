#include <corelib/core/Project.h>
#include <corelib/Exception/Exception.h>
#include <corelib/core/coresql.h>
#include <corelib/core/PathParser.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>

#include <corelib/coreOrm/doodleConfig_sqlOrm.h>
#include <corelib/coreOrm/PathParserData_sqlOrm.h>
#include <corelib/coreOrm/zh_ch_sqlOrm.h>
#include <corelib/coreOrm/class_alias_sqlOrm.h>

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
                 .from(table_config)
                 .where(table_config.doodleKey == sqlpp::parameter(table_config.doodleKey));
  auto sql_ps = db->prepare(sql);

  sql_ps.params.doodleKey = "Root";
  for (auto&& row : (*db)(sql_ps))
    p_root = row.doodleValue.value();

  sql_ps.params.doodleKey = "Name";
  for (auto&& row : (*db)(sql_ps))
    p_name = row.doodleValue.value();

  sql_ps.params.doodleKey = "assRoot";
  for (auto&& row : (*db)(sql_ps))
    p_ass_Root.emplace_back(std::make_shared<fileSys::path>(row.doodleValue.value()));

  sql_ps.params.doodleKey = "shotRoot";
  for (auto&& row : (*db)(sql_ps))
    p_shotRoot.emplace_back(std::make_shared<fileSys::path>(row.doodleValue.value()));

  if (p_root != path) throw std::runtime_error("目录无法对应");

  auto sql2 = sqlpp::select(sqlpp::all_of(table_parser))
                  .from(table_parser)
                  .order_by(table_parser.id.asc(), table_parser.parent.asc())
                  .unconditionally();

  for (auto&& row : (*db)(sql2)) {
    auto k_parser      = std::make_shared<pathParser::PathParser>();
    k_parser->p_id     = row.id.value();
    k_parser->p_class  = row.refClass.value();
    k_parser->p_prefix = row.prefix.value();
    k_parser->p_regex  = row.regex.value();
    k_parser->p_suffix = row.suffix.value();
    p_path_parsers.insert({k_parser->p_id, k_parser});
    if (!row.parent.is_null()) {
      auto it                    = p_path_parsers.find(row.parent.value());
      it->second->p_child_parser = k_parser;
    }
  }
}

Project::pathParserList Project::findParser(const rttr::type& type_ptr) {
  pathParserList k_parser_list{};
  for (auto&& p : p_path_parsers) {
    if (p.second->p_class == type_ptr.get_name()) {
      k_parser_list.push_back(p.second);
    }
  }
  return k_parser_list;
}

const std::map<int64_t, std::string> Project::getClassNames(const rttr::type& type_ptr) const {
  auto db = coreSql::getCoreSql().getConnection();
  DoodleConfig table_config{};

  auto sql = sqlpp::select(sqlpp::all_of(table_config))
                 .from(table_config)
                 .where(table_config.doodleKey == std::string{type_ptr.get_name()});
  std::map<int64_t, std::string> k_list;

  for (auto&& row : (*db)(sql)) {
    k_list.insert({row.id.value(), row.doodleValue.value()});
  }
  if (k_list.empty()) throw nullptr_error("not name");
  return k_list;
}

const std::multimap<std::string, std::string> Project::getAlias(const rttr::type& type_ptr) const {
  auto db = coreSql::getCoreSql().getConnection();
  DoodleConfig table_config{};
  ClassAlias table{};

  auto k_list = getClassNames(type_ptr);
  std::multimap<std::string, std::string> k_reply;

  auto sql2 = sqlpp::select(sqlpp::all_of(table))
                  .from(table)
                  .where(table.classIndex == sqlpp::parameter(table.id));

  auto sql2_ps = db->prepare(sql2);
  for (auto&& item : k_list) {
    sql2_ps.params.id = item.first;
    for (auto&& row : (*db)(sql2_ps))
      k_reply.insert({item.second, row.doodleAlias.value()});
  }
  return k_reply;
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