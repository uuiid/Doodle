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
void database::set_id(std::uint64_t in_id) const {
  p_id     = in_id;
  p_id_str = fmt::format("id {}", p_id);
}

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

database::database()
    : p_id(0),
      p_id_str("id 0"),
      p_parent_id(),
      p_type(metadata_type::unknown_file),
      p_uuid_(core_set::getSet().get_uuid()),
      p_uuid(),
      p_boost_serialize_vesion(0) {
  p_uuid = boost::uuids::to_string(p_uuid_);
}

database::~database() = default;

DOODLE_MOVE_CPP(database);

void database::set_enum(entt::registry &in_reg, entt::entity in_ent) {
  auto k_h        = entt::handle{in_reg, in_ent};
  auto [k_p, k_f] = k_h.try_get<project, assets_file>();
  auto &k_data    = k_h.get<database>();

  if (k_p)
    k_data.p_type = metadata_type::project_root;
  else if (k_f) {
    k_data.p_type = metadata_type::file;
  } else
    k_data.p_type = metadata_type::folder;
}

FSys::path database::get_url_uuid() const {
  auto k_h     = make_handle(*this);

  auto l_reg   = g_reg();

  // 找到根的数据库类
  auto &k_data = k_h.get<root_ref>().root_handle().get<database>();

  // 组合路径
  auto path    = FSys::path{k_data.p_uuid};
  path /= p_uuid.substr(0, 3);
  path /= p_uuid;
  return path;
}

void database::set_meta_type(const metadata_type &in_meta) {
  p_type = in_meta;
};

void database::set_meta_type(const std::string &in_meta) {
  p_type = magic_enum::enum_cast<metadata_type>(in_meta)
               .value_or(metadata_type::unknown_file);
};

void database::set_meta_type(std::int32_t in_) {
  p_type = magic_enum::enum_cast<metadata_type>(in_)
               .value_or(metadata_type::unknown_file);
};

std::int32_t database::get_meta_type_int() const {
  return magic_enum::enum_integer(p_type);
}

std::uint64_t database::get_id() const {
  return p_id;
}

bool database::is_install() const {
  return p_id > 0;
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
  auto k_h    = make_handle(*this);
  /// 转换序列化数据
  auto k_data = in_.metadata_cereal().value();
  try {
    auto k_json = nlohmann::json ::parse(k_data);
    *this       = k_json["database"];
    entt_tool::load_comm<DOODLE_SERIALIZATION>(k_h, k_json);
  } catch (const nlohmann::json::parse_error &e) {
    DOODLE_LOG_ERROR(e.what());
  }

  /// 转换id
  set_id(in_.id());
  /// 转化类型
  p_type = magic_enum::enum_cast<metadata_type>(in_.m_type().value())
               .value_or(metadata_type::unknown_file);

  /// 确认转换可索引数据

  return *this;
}

bool database::has_components() const {
  auto k_h = make_handle(*this);
  return k_h.any_of<DOODLE_SERIALIZATION>();
}

database::operator doodle::metadata_database() const {
  auto k_h = make_handle(*this);

  if (!k_h.any_of<DOODLE_SERIALIZATION>())
    throw serialization_error{"空组件"};

  metadata_database k_tmp{};
  ///转换id
  if (p_id != 0)
    k_tmp.set_id(p_id);
  ///设置序列化数据储存位置
  k_tmp.set_uuid_path(get_url_uuid().generic_string());

  nlohmann::json k_json{};
  {
    k_json["database"] = *this;
    entt_tool::save_comm<DOODLE_SERIALIZATION>(k_h, k_json);
  }

  k_tmp.mutable_metadata_cereal()->set_value(k_json.dump());
  if (p_parent_id)
    k_tmp.mutable_parent()->set_value(*p_parent_id);

  ///设置类型id
  k_tmp.mutable_m_type()->set_value(get_meta_type_int());
  /// 设置可索引数据
  if (k_h.any_of<season>()) {
    k_tmp.mutable_season()->set_value(k_h.get<season>().get_season());
  }
  if (k_h.any_of<episodes>()) {
    k_tmp.mutable_episode()->set_value(k_h.get<episodes>().get_episodes());
  }
  if (k_h.any_of<shot>()) {
    k_tmp.mutable_episode()->set_value(k_h.get<shot>().get_shot());
  }
  if (k_h.any_of<assets>()) {
    k_tmp.mutable_assets()->set_value(k_h.get<assets>().get_path().generic_string());
  }

  return k_tmp;
}

#undef DOODLE_SERIALIZATION

bool database::operator==(const database &in_rhs) const {
  return std::tie(p_id, p_uuid_) == std::tie(in_rhs.p_id, in_rhs.p_uuid_);
}

bool database::operator!=(const database &in_rhs) const {
  return !(*this == in_rhs);
}

const boost::uuids::uuid &database::uuid() const {
  return p_uuid_;
}

const string &database::get_id_str() const {
  return p_id_str;
}
bool database::operator==(const boost::uuids::uuid &in_rhs) const {
  return p_uuid_ == in_rhs;
}
bool database::operator!=(const boost::uuids::uuid &in_rhs) const {
  return !(*this == in_rhs);
}

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

}  // namespace doodle
