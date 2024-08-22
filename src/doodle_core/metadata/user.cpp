//
// Created by TD on 2021/5/7.
//

#include <doodle_core/core/core_set.h>
#include <doodle_core/database_task/details/column.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/sqlite3/sqlite3.h>
#include <doodle_core/metadata/detail/user_set_data.h>
#include <doodle_core/metadata/user.h>

#include <boost/preprocessor.hpp>

#include "exception/exception.h"
#include "metadata/metadata.h"
#include <entt/entt.hpp>
#include <pin_yin/convert.h>
#include <sqlpp11/ppgen.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
namespace doodle {

void to_json(nlohmann::json& j, const user& p) {
  j["string_"] = p.p_string_;
  j["power"]   = p.power;
}
void from_json(const nlohmann::json& j, user& p) {
  j.at("string_").get_to(p.p_string_);
  p.p_ENUS = convert::Get().toEn(p.p_string_);
  if (j.contains("power")) j.at("power").get_to(p.power);
}

user::user() : p_string_(), p_ENUS() {}

user::user(const std::string& in_string) : user() { set_name(in_string); }

const std::string& user::get_name() const { return p_string_; }
void user::set_name(const std::string& in_string) {
  p_string_ = in_string;
  p_ENUS    = convert::Get().toEn(p_string_);
}

const std::string& user::get_enus() const { return p_ENUS; }
bool user::operator==(const user& in_rhs) const { return p_string_ == in_rhs.p_string_; }
bool user::operator<(const user& in_rhs) const { return p_string_ < in_rhs.p_string_; }

entt::handle user::find_by_user_name(const std::string& in_name) {
  entt::handle l_r{};
  for (auto&& [e, u] : g_reg()->view<user>().each()) {
    if (u.get_name() == in_name) {
      l_r = entt::handle{*g_reg(), e};
      break;
      DOODLE_LOG_WARN("找到用户名称 {} 的句柄 {}", in_name, l_r);
    }
  }
  return l_r;
}

user::current_user::operator entt::handle() { return get_handle(); }
entt::handle user::current_user::get_handle() {
  if (!user_handle) {
    user_handle = database::find_by_uuid(uuid);
  }

  if (!*this) {
    auto l_create_h = entt::handle{*g_reg(), g_reg()->create()};
    l_create_h.emplace<user>(core_set::get_set().user_name);
    l_create_h.emplace<business::rules>(business::rules::get_default());
    uuid        = l_create_h.emplace<database>(uuid).uuid();
    user_handle = l_create_h;
  }

  //  DOODLE_CHICK(
  //      user_handle && user_handle.any_of<database>() && user_handle.get<database>() == uuid,
  //      doodle_error{"缺失用户实体{}", user_handle}
  //  );
  return user_handle;
}

std::string user::current_user::user_name_attr() {
  if (!*this) get_handle();
  return user_handle.get<user>().get_name();
}
void user::current_user::user_name_attr(const std::string& in_name) {
  if (!*this) get_handle();
  user_handle.patch<user>().set_name(in_name);
  core_set::get_set().user_name = in_name;
}

void user::current_user::set_user(const entt::handle& in) {
  if (!in.all_of<user, database>()) throw_exception(doodle_error{"句柄缺失"});
  const auto& [l_user, l_d]     = in.get<const user, const database>();
  core_set::get_set().user_id   = l_d.uuid();
  core_set::get_set().user_name = l_user.get_name();
  user_handle                   = in;
  uuid                          = l_d.uuid();
}

user::current_user::operator bool() const {
  return user_handle && user_handle.all_of<database, user>() && user_handle.get<database>() == uuid;
}
user::current_user::current_user() : uuid(core_set::get_set().user_id) {}

user::current_user::~current_user() = default;
void user::current_user::create_user() {
  auto l_create_h = entt::handle{*g_reg(), g_reg()->create()};
  l_create_h.emplace<user>(core_set::get_set().user_name);
  l_create_h.emplace<business::rules>(business::rules::get_default());
  uuid                        = l_create_h.emplace<database>().uuid();
  core_set::get_set().user_id = uuid;
  user_handle                 = l_create_h;
}

namespace {
DOODLE_SQL_COLUMN_IMP(uuid, sqlpp::blob, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(user_name, sqlpp::text, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(mobile, sqlpp::text, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(dingding_id, sqlpp::text, database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(dingding_company_id, sqlpp::blob, database_n::detail::can_be_null);

DOODLE_SQL_TABLE_IMP(user_tab, tables::column::id, uuid, user_name, mobile, dingding_id, dingding_company_id);
}  // namespace

std::vector<user> user::select_all(const sql_connection_ptr& in_comm) {
  user_tab l_tab{};
  std::vector<user> l_r{};
  for (const auto& l_row : (*in_comm)(sqlpp::select(sqlpp::all_of(l_tab)).from(l_tab).unconditionally())) {
    user l_user{};
    std::copy_n(l_row.uuid.value().begin(), l_user.id_.size(), l_user.id_.begin());
    l_user.p_string_    = l_row.user_name.value();
    l_user.mobile_      = l_row.mobile.value();
    l_user.dingding_id_ = l_row.dingding_id.value();
    if (!l_row.dingding_company_id.is_null())
      std::copy_n(
          l_row.dingding_company_id.value().begin(), l_user.dingding_company_id_.size(),
          l_user.dingding_company_id_.begin()
      );
    l_r.emplace_back(std::move(l_user));
  }
  return l_r;
}

std::map<std::int64_t, boost::uuids::uuid> user::select_all_map_id(const sql_connection_ptr& in_comm) {
  user_tab l_tab{};
  std::map<std::int64_t, boost::uuids::uuid> l_r{};
  for (const auto& l_row : (*in_comm)(sqlpp::select(l_tab.id, l_tab.uuid).from(l_tab).unconditionally())) {
    boost::uuids::uuid l_uuid{};
    std::copy_n(l_row.uuid.value().begin(), l_uuid.size(), l_uuid.begin());
    l_r.emplace(l_row.id.value(), l_uuid);
  }
  return l_r;
}

void user::create_table(const sql_connection_ptr& in_comm) {
  in_comm->execute(R"(
    CREATE TABLE IF NOT EXISTS user_tab (
        id                  INTEGER PRIMARY KEY AUTOINCREMENT,
        uuid                BLOB    NOT NULL   UNIQUE,
        user_name           TEXT,
        mobile              TEXT,
        dingding_id         TEXT,
        dingding_company_id BLOB
    );
  )");
  in_comm->execute(R"(
    CREATE INDEX IF NOT EXISTS user_tab_uuid_index ON user_tab (uuid);
  )");
}

std::vector<bool> user::filter_exist(const sql_connection_ptr& in_comm, const std::vector<user>& in_task) {
  user_tab l_tab{};
  std::vector<bool> l_r{};
  auto l_pre = in_comm->prepare(
      sqlpp::select(sqlpp::count(l_tab.id)).from(l_tab).where(l_tab.uuid == sqlpp::parameter(l_tab.uuid))
  );
  for (const auto& l_user : in_task) {
    l_pre.params.uuid = {l_user.id_.begin(), l_user.id_.end()};
    for (const auto& l_row : (*in_comm)(l_pre)) {
      l_r.emplace_back(l_row.count.value() > 0);
      break;
    }
  }
  return l_r;
}
void user::insert(const sql_connection_ptr& in_comm, const std::vector<user>& in_task) {
  user_tab l_tab{};
  auto l_pre = in_comm->prepare(sqlpp::insert_into(l_tab).set(
      l_tab.uuid = sqlpp::parameter(l_tab.uuid), l_tab.user_name = sqlpp::parameter(l_tab.user_name),
      l_tab.mobile = sqlpp::parameter(l_tab.mobile), l_tab.dingding_id = sqlpp::parameter(l_tab.dingding_id),
      l_tab.dingding_company_id = sqlpp::parameter(l_tab.dingding_company_id)
  ));
  for (const auto& l_user : in_task) {
    l_pre.params.uuid                = {l_user.id_.begin(), l_user.id_.end()};
    l_pre.params.user_name           = l_user.p_string_;
    l_pre.params.mobile              = l_user.mobile_;
    l_pre.params.dingding_id         = l_user.dingding_id_;
    l_pre.params.dingding_company_id = {l_user.dingding_company_id_.begin(), l_user.dingding_company_id_.end()};
    (*in_comm)(l_pre);
  }
}
void user::update(const sql_connection_ptr& in_comm, const std::vector<user>& in_task) {
  user_tab l_tab{};
  auto l_pre = in_comm->prepare(sqlpp::update(l_tab)
                                   .set(
                                       l_tab.user_name           = sqlpp::parameter(l_tab.user_name),
                                       l_tab.mobile              = sqlpp::parameter(l_tab.mobile),
                                       l_tab.dingding_id         = sqlpp::parameter(l_tab.dingding_id),
                                       l_tab.dingding_company_id = sqlpp::parameter(l_tab.dingding_company_id)
                                   )
                                   .where(l_tab.uuid == sqlpp::parameter(l_tab.uuid)));
  for (const auto& l_user : in_task) {
    l_pre.params.user_name           = l_user.p_string_;
    l_pre.params.mobile              = l_user.mobile_;
    l_pre.params.dingding_id         = l_user.dingding_id_;
    l_pre.params.uuid                = {l_user.id_.begin(), l_user.id_.end()};
    l_pre.params.dingding_company_id = {l_user.dingding_company_id_.begin(), l_user.dingding_company_id_.end()};
    (*in_comm)(l_pre);
  }
}
void user::delete_by_ids(const sql_connection_ptr& in_comm, const std::vector<boost::uuids::uuid>& in_ids) {
  user_tab l_tab{};
  auto l_pre = in_comm->prepare(sqlpp::remove_from(l_tab).where(l_tab.uuid == sqlpp::parameter(l_tab.uuid)));
  for (const auto& l_id : in_ids) {
    l_pre.params.uuid = {l_id.begin(), l_id.end()};
    (*in_comm)(l_pre);
  }
}

}  // namespace doodle
