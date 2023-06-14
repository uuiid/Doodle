//
// Created by teXiao on 2021/4/27.
//

#include "metadata.h"

#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/importance.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/redirection_path_info.h>

#include <boost/hana/ext/std.hpp>

#include <core/core_set.h>

namespace doodle {

namespace database_ns {
ref_data::ref_data() = default;

ref_data::ref_data(const database &in) : uuid(in.uuid()) {}
bool ref_data::operator==(const database::ref_data &in_rhs) const { return uuid == in_rhs.uuid; }

void from_json(const nlohmann::json &j, database::ref_data &p) {
  if (j.contains("uuid")) j["uuid"].get_to(p.uuid);
}
void to_json(nlohmann::json &j, const database::ref_data &p) { j["uuid"] = p.uuid; }
ref_data::operator bool() const {
  bool l_r{false};

  //  ranges::make_subrange(g_reg()->view<database>().each());

  for (auto &&[e, d] : g_reg()->view<database>().each())
    if (d == uuid) {
      l_r = true;
      break;
    }
  return l_r;
}

entt::handle ref_data::handle() const {
  entt::handle l_r{};

  //  ranges::make_subrange(g_reg()->view<database>().each());
  if (!uuid.is_nil())
    for (auto &&[e, d] : g_reg()->view<database>().each())
      if (d == uuid) {
        l_r = entt::handle{*g_reg(), e};
        break;
      }
  return l_r;
}
}  // namespace database_ns

database::database() : p_id(0), p_uuid_(core_set::get_set().get_uuid()) {}
database::database(const boost::uuids::uuid &in_uuid) : p_id(0), p_uuid_(in_uuid) {}
database::database(const std::string &in_uuid_str)
    : p_id(0), p_uuid_(boost::lexical_cast<boost::uuids::uuid>(in_uuid_str)) {}

database::~database() = default;

std::uint64_t database::get_id() const { return p_id; }

bool database::is_install() const { return p_id > 0; }

bool database::operator==(const database &in_rhs) const { return p_uuid_ == in_rhs.p_uuid_; }
bool database::operator==(const boost::uuids::uuid &in_rhs) const { return p_uuid_ == in_rhs; }
bool database::operator==(const database_ns::ref_data &in_rhs) const { return p_uuid_ == in_rhs.uuid; }

void database::set_id(std::uint64_t in_id) const { p_id = in_id; }
const boost::uuids::uuid &database::uuid() const { return p_uuid_; }

entt::handle database::find_by_uuid(const boost::uuids::uuid &in) {
  entt::handle l_r{};
  for (auto &&[e, d] : g_reg()->view<database>().each())
    if (d == in) {
      l_r = entt::handle{*g_reg(), e};
      break;
    }
  return l_r;
}
entt::handle database::find_by_uuid() const { return find_by_uuid(uuid()); }

void to_json(nlohmann::json &j, const database &p) {
  j["uuid"] = p.p_uuid_;
  j["id"]   = p.p_id;
}
void from_json(const nlohmann::json &j, database &p) {
  j["uuid"].get_to(p.p_uuid_);
  j["id"].get_to(p.p_id);
}

}  // namespace doodle
