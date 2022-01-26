//
// Created by teXiao on 2021/4/27.
//

#include "metadata.h"

#include <doodle_lib/core/ContainerDevice.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/metadata/metadata_factory.h>
#include <exception/exception.h>
#include <google/protobuf/util/time_util.h>
// clang-format off
#include <metadata/metadata_cpp.h>
#include <doodle_lib/lib_warp/entt_warp.h>
// clang-format on

#include <boost/hana/ext/std.hpp>
namespace doodle {

const std::uint64_t &database_root::get_current_id() const {
  return p_current_id;
}

bool database_root::is_end() const {
  return p_end;
}

void database_root::reset() {
  p_current_id = 0;
  p_cout_rows  = 0;
  p_end        = false;
}

class database::impl {
 public:
  impl()
      : p_id(0),
        p_id_str("id 0"),
        p_parent_id(),
        p_type(metadata_type::unknown_file),
        p_uuid_(core_set::getSet().get_uuid()),
        p_uuid(boost::uuids::to_string(p_uuid_)) {
  }
  mutable std::uint64_t p_id;
  mutable string p_id_str;
  std::optional<uint32_t> p_parent_id;
  metadata_type p_type;
  boost::uuids::uuid p_uuid_;
  FSys::path p_uuid;
};

database::database()
    : p_i(std::make_unique<impl>()),
      status_(status::none) {
}

database::~database() = default;

void database::set_enum(entt::registry &in_reg, entt::entity in_ent) {
  auto k_h     = entt::handle{in_reg, in_ent};
  auto &k_data = k_h.get<database>();

  if (k_h.any_of<project>())
    k_data.p_i->p_type = metadata_type::project_root;
  else if (k_h.any_of<assets_file>()) {
    k_data.p_i->p_type = metadata_type::file;
  } else
    k_data.p_i->p_type = metadata_type::folder;
}

const FSys::path &database::get_url_uuid() const {
  return p_i->p_uuid;
}

void database::set_meta_type(const metadata_type &in_meta) {
  p_i->p_type = in_meta;
};

void database::set_meta_type(const std::string &in_meta) {
  p_i->p_type = magic_enum::enum_cast<metadata_type>(in_meta)
                    .value_or(metadata_type::unknown_file);
};

void database::set_meta_type(std::int32_t in_) {
  p_i->p_type = magic_enum::enum_cast<metadata_type>(in_)
                    .value_or(metadata_type::unknown_file);
};

std::int32_t database::get_meta_type_int() const {
  return magic_enum::enum_integer(p_i->p_type);
}

std::uint64_t database::get_id() const {
  return p_i->p_id;
}

bool database::is_install() const {
  return p_i->p_id > 0;
}

#define DOODLE_SERIALIZATION project,            \
                             episodes,           \
                             shot,               \
                             season,             \
                             assets,             \
                             assets_file,        \
                             assets_path_vector, \
                             time_point_wrap,    \
                             comment_vector,     \
                             project::cloth_config

database &database::operator=(const metadata_database &in_) {
  auto k_h = make_handle(*this);
  /// 转换序列化数据
  try {
    auto k_json = nlohmann::json ::parse(in_.user_data);
    k_json["database"].get_to(*this);
    entt_tool::load_comm<DOODLE_SERIALIZATION>(k_h, k_json);
  } catch (const nlohmann::json::parse_error &e) {
    DOODLE_LOG_ERROR(e.what());
  }

  /// 转换id
  set_id(in_.id);
  /// 转化类型
  p_i->p_type = magic_enum::enum_cast<metadata_type>(in_.m_type)
                    .value_or(metadata_type::unknown_file);
  /// 确认转换可索引数据

  return *this;
}

bool database::has_components() const {
  auto k_h = make_handle(*this);
  return k_h.any_of<DOODLE_SERIALIZATION>();
}

database::operator metadata_database() const {
  auto k_h = make_handle(*this);

  chick_true<serialization_error>(k_h.any_of<DOODLE_SERIALIZATION>(), DOODLE_LOC, "组件缺失");

  metadata_database k_tmp{};
  ///转换id
  if (p_i->p_id != 0)
    k_tmp.id = p_i->p_id;
  ///设置序列化数据储存位置
  k_tmp.uuid_path = get_url_uuid().generic_string();

  nlohmann::json k_json{};
  {
    k_json["database"] = *this;
    entt_tool::save_comm<DOODLE_SERIALIZATION>(k_h, k_json);
  }

  k_tmp.user_data = k_json.dump();
  if (auto k_data = g_reg()->try_ctx<database>(); k_data)
    k_tmp.parent = boost::numeric_cast<std::uint32_t>(k_data->get_id());

  ///设置类型id
  k_tmp.m_type = get_meta_type_int();
  /// 设置可索引数据
  if (k_h.any_of<season>()) {
    k_tmp.season = k_h.get<season>().get_season();
  }
  if (k_h.any_of<episodes>()) {
    k_tmp.episode = k_h.get<episodes>().get_episodes();
  }
  if (k_h.any_of<shot>()) {
    k_tmp.shot = k_h.get<shot>().get_shot();
  }
  if (k_h.any_of<assets>()) {
    k_tmp.assets = k_h.get<assets>().get_path().generic_string();
  }

  return k_tmp;
}

#undef DOODLE_SERIALIZATION

bool database::operator==(const database &in_rhs) const {
  return std::tie(p_i->p_id, p_i->p_uuid_) == std::tie(in_rhs.p_i->p_id, in_rhs.p_i->p_uuid_);
}

bool database::operator!=(const database &in_rhs) const {
  return !(*this == in_rhs);
}
void database::set_id(std::uint64_t in_id) const {
  p_i->p_id     = in_id;
  p_i->p_id_str = fmt::format("id {}", p_i->p_id);
}
const boost::uuids::uuid &database::uuid() const {
  return p_i->p_uuid_;
}

const string &database::get_id_str() const {
  return p_i->p_id_str;
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
// database::database(const metadata_database &in_metadata_database) {
// }

const string &to_str::get() const {
  auto k_h   = make_handle(*this);
  auto k_tup = k_h.try_get<project,
                           episodes,
                           shot,
                           season,
                           assets,
                           assets_file>();

  boost::hana::for_each(k_tup, [&](auto ptr) {
    if (ptr)
      p_str = ptr->str();
  });
  return p_str;
}

to_str::operator string() const {
  return get();
}
void from_json(const nlohmann::json &j, database &p) {
  j.at("id").get_to(p.p_i->p_id);
  j.at("parent_id").get_to(p.p_i->p_parent_id);
  j.at("type").get_to(p.p_i->p_type);
  j.at("uuid_").get_to(p.p_i->p_uuid_);
  p.p_i->p_uuid = boost::uuids::to_string(p.p_i->p_uuid_);
}
void to_json(nlohmann::json &j, const database &p) {
  j["id"]        = p.p_i->p_id;
  j["parent_id"] = p.p_i->p_parent_id;
  j["type"]      = p.p_i->p_type;
  j["uuid_"]     = p.p_i->p_uuid_;
}
}  // namespace doodle
