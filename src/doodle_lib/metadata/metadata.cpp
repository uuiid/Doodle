//
// Created by teXiao on 2021/4/27.
//

#include "metadata.h"

#include <doodle_lib/core/ContainerDevice.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/logger/logger.h>
#include <exception/exception.h>
#include <google/protobuf/util/time_util.h>

// clang-format off
#include <metadata/metadata_cpp.h>
#include <metadata/image_icon.h>
#include <doodle_lib/lib_warp/entt_warp.h>
// clang-format on

#include <boost/hana/ext/std.hpp>
namespace doodle {

class database::impl {
 public:
  impl()
      : p_id(0),
        p_parent_id(),
        p_type(metadata_type::unknown_file),
        p_uuid_(core_set::getSet().get_uuid()) {
  }
  mutable std::uint64_t p_id;
  std::optional<uint32_t> p_parent_id;
  metadata_type p_type;
  boost::uuids::uuid p_uuid_;
};

database::ref_data::ref_data(const database &in)
    : id(in.p_i->p_id),
      uuid(in.p_i->p_uuid_) {
}

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

#define DOODLE_SERIALIZATION project,               \
                             episodes,              \
                             shot,                  \
                             season,                \
                             assets,                \
                             assets_file,           \
                             time_point_wrap,       \
                             comment,               \
                             std::vector<comment>,  \
                             project::cloth_config, \
                             image_icon

database &database::operator=(const metadata_database &in_) {
  auto k_h = make_handle(*this);
  /// 转换序列化数据
  try {
    auto k_json = nlohmann::json ::parse(in_.user_data);
    entt_tool::load_comm<DOODLE_SERIALIZATION>(k_h, k_json);
  } catch (const nlohmann::json::parse_error &e) {
    DOODLE_LOG_ERROR(e.what());
  }

  /// 转换id
  set_id(in_.id);
  if (in_.parent)
    p_i->p_parent_id = *in_.parent;
  /// 转化类型
  p_i->p_type = magic_enum::enum_cast<metadata_type>(in_.m_type)
                    .value_or(metadata_type::unknown_file);
  if (!in_.uuid_.is_nil())
    p_i->p_uuid_ = in_.uuid_;

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

  /// 储存对应数据字符串的位置
  if (k_h.any_of<assets_file>())
    k_tmp.uuid_path = k_h.get<assets_file>().path.generic_string();

  nlohmann::json k_json{};
  {
    entt_tool::save_comm<DOODLE_SERIALIZATION>(k_h, k_json);
  }

  k_tmp.user_data = k_json.dump();
  if (p_i->p_type != metadata_type::project_root)
    k_tmp.parent = boost::numeric_cast<std::uint32_t>(g_reg()->ctx<ref_data>().id);

  ///设置类型id
  k_tmp.m_type = get_meta_type_int();
  /// 设置可索引数据
  if (k_h.any_of<season>()) {
    k_tmp.season = k_h.get<season>().get_season();
  }
  if (k_h.any_of<episodes>()) {
    k_tmp.episode = boost::numeric_cast<std::int32_t>(k_h.get<episodes>().get_episodes());
  }
  if (k_h.any_of<shot>()) {
    k_tmp.shot = boost::numeric_cast<std::int32_t>(k_h.get<shot>().get_shot());
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

}  // namespace doodle
