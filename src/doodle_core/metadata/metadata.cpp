//
// Created by teXiao on 2021/4/27.
//

#include "metadata.h"

#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/importance.h>
#include <doodle_core/metadata/organization.h>
#include <doodle_core/metadata/redirection_path_info.h>
#include <doodle_core/lib_warp/entt_warp.h>

#include <core/core_set.h>

#include <boost/hana/ext/std.hpp>
namespace doodle {

class database::impl {
 public:
  impl()
      : p_id(0),
        p_uuid_(core_set::getSet().get_uuid()) {
  }
  impl(const std::string &in_uuid_str)
      : p_id(0),
        p_uuid_(boost::lexical_cast<boost::uuids::uuid>(in_uuid_str)) {
  }
  mutable std::uint64_t p_id;
  boost::uuids::uuid p_uuid_;
};

database::ref_data::ref_data(const database &in)
    : id(in.p_i->p_id),
      uuid(in.p_i->p_uuid_) {
}
void from_json(const nlohmann::json &j, database::ref_data &p) {
  if (j.contains("uuid"))
    j["uuid"].get_to(p.uuid);
}
void to_json(nlohmann::json &j, const database::ref_data &p) {
  j["uuid"] = p.uuid;
}
bool database::ref_data::operator==(const database::ref_data &in_rhs) const {
  return uuid == in_rhs.uuid;
}
bool database::ref_data::operator!=(const database::ref_data &in_rhs) const {
  return !(in_rhs == *this);
}
database::ref_data::operator bool() const {
  bool l_r{false};

  //  ranges::make_subrange(g_reg()->view<database>().each());

  for (auto &&[e, d] : g_reg()->view<database>().each())
    if (d == uuid) {
      l_r = true;
      break;
    }
  return l_r;
}

entt::handle database::ref_data::handle() const {
  entt::handle l_r{};

  //  ranges::make_subrange(g_reg()->view<database>().each());

  for (auto &&[e, d] : g_reg()->view<database>().each())
    if (d == uuid) {
      l_r = entt::handle{*g_reg(), e};
      break;
    }
  return l_r;
}
bool database::ref_data::find_for_path(const FSys::path &in_path) {
  bool l_r{false};
  for (auto &&[e, a] : g_reg()->view<assets_file>().each()) {
    if (a.path == in_path) {
      uuid = make_handle(e).get<database>().uuid();
      l_r  = true;
      break;
    }
  }
  return l_r;
}
database::ref_data::ref_data() = default;
database::database()
    : p_i(std::make_unique<impl>()),
      status_(status::none) {
}
database::database(const std::string &in_uuid_str)
    : p_i(std::make_unique<impl>(in_uuid_str)),
      status_(status::none) {
}
database::~database() = default;

std::uint64_t database::get_id() const {
  return p_i->p_id;
}

bool database::is_install() const {
  return p_i->p_id > 0;
}

bool database::operator==(const database &in_rhs) const {
  return std::tie(p_i->p_id, p_i->p_uuid_) == std::tie(in_rhs.p_i->p_id, in_rhs.p_i->p_uuid_);
}

bool database::operator!=(const database &in_rhs) const {
  return !(*this == in_rhs);
}
void database::set_id(std::uint64_t in_id) const {
  p_i->p_id = in_id;
}
const boost::uuids::uuid &database::uuid() const {
  return p_i->p_uuid_;
}

bool database::operator==(const boost::uuids::uuid &in_rhs) const {
  return p_i->p_uuid_ == in_rhs;
}
bool database::operator!=(const boost::uuids::uuid &in_rhs) const {
  return !(*this == in_rhs);
}
database::database(database &&in) noexcept
    : database() {
  p_i.swap(in.p_i);
  this->status_ = in.status_.load();
}
database &database::operator=(database &&in) noexcept {
  p_i.swap(in.p_i);
  this->status_ = in.status_.load();
  return *this;
}

bool database::operator==(const ref_data &in_rhs) const {
  return std::tie(p_i->p_id, p_i->p_uuid_) == std::tie(in_rhs.id, in_rhs.uuid);
}
bool database::operator!=(const ref_data &in_rhs) const {
  return !(*this == in_rhs);
}

bool database::operator==(const std::string &in_rhs) const {
  return p_i->p_uuid_ == boost::lexical_cast<boost::uuids::uuid>(in_rhs);
}
bool database::operator!=(const std::string &in_rhs) const {
  return !(*this == in_rhs);
}

}  // namespace doodle
