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
#include <metadata/metadata_cpp.h>

#include <boost/algorithm/algorithm.hpp>
#include <boost/archive/polymorphic_text_iarchive.hpp>
#include <boost/archive/polymorphic_text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/hana/ext/std.hpp>
#include <boost/range/algorithm/count_if.hpp>

namespace doodle {
void tree_relationship::set_parent_raw(const entt::handle &in_parent) {
  p_parent     = in_parent;
  auto &k_tree = in_parent.get<tree_relationship>();
  k_tree.p_child.push_back(to_entity(*this));
}
entt::entity tree_relationship::get_parent() const noexcept {
  return p_parent;
}

entt::handle tree_relationship::get_parent_h() const noexcept {
  return make_handle(p_parent);
}

void tree_relationship::set_parent(const entt::entity &in_parent) noexcept {
  auto l_old_p_h = make_handle(p_parent);
  auto this_h    = make_handle(*this);
  if (l_old_p_h && l_old_p_h.all_of<tree_relationship, database>()) {
    boost::remove_erase_if(l_old_p_h.get<tree_relationship>().p_child,
                           [&](auto in) {
                             return this_h.entity() == in;
                           });
    auto &k_d = l_old_p_h.get<database>();
    --(k_d.p_has_child);
    if (this_h.all_of<assets_file>())
      --(k_d.p_has_file);
  }

  p_parent   = in_parent;
  auto l_p_h = make_handle(in_parent);
  if (l_p_h.all_of<tree_relationship, database>()) {
    l_p_h.get<tree_relationship>().p_child.push_back(this_h);
    auto &k_d                          = l_p_h.get<database>();
    // 设置父id
    this_h.get<database>().p_parent_id = k_d.get_id();
    // 设置父物体中储存的子数据
    ++(k_d.p_has_child);
    if (this_h.all_of<assets_file>())
      ++(k_d.p_has_file);
  }
}

const std::vector<entt::entity> &tree_relationship::get_child() const noexcept {
  return p_child;
}

// std::vector<entt::entity> &tree_relationship::get_child() noexcept {
//   return p_child;
// }

// void tree_relationship::set_child(const std::vector<entt::entity> &in_child) noexcept {
//   p_child = in_child;
// }

entt::handle tree_relationship::get_root() const {
  auto k_h = make_handle(*this);
  while (k_h) {
    auto &k_tree = k_h.get<tree_relationship>();
    if (!k_tree.has_parent())
      return k_h;
    k_h = k_tree.get_parent_h();
  }
  return k_h;
}
bool tree_relationship::has_parent() const {
  return p_parent != entt::null;
}

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

database::database()
    : p_id(0),
      p_id_str("id 0"),
      p_parent_id(),
      p_type(metadata_type::unknown_file),
      p_uuid(core_set::getSet().get_uuid_str()),
      p_has_child(0),
      p_has_file(0),
      p_updata_parent_id(false),
      p_updata_type(false),
      p_boost_serialize_vesion(0) {
}

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
  auto l_ent   = entt::to_entity(*l_reg, *this);

  // 找到根的数据库类
  auto &k_data = k_h.get<root_ref>().root_handle().get<database>();

  // 组合路径
  auto path    = FSys::path{k_data.p_uuid};
  path /= p_uuid.substr(0, 3);
  path /= p_uuid;
  return path;
}

bool database::has_parent() const {
  return p_parent_id.has_value();
}

void database::set_meta_type(const metadata_type &in_meta) {
  p_type        = in_meta;
  p_updata_type = true;
};

void database::set_meta_type(const std::string &in_meta) {
  p_type = magic_enum::enum_cast<metadata_type>(in_meta)
               .value_or(metadata_type::unknown_file);
  p_updata_type = true;
};

void database::set_meta_type(std::int32_t in_) {
  p_type = magic_enum::enum_cast<metadata_type>(in_)
               .value_or(metadata_type::unknown_file);
  p_updata_type = true;
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
                             comment_vector

database &database::operator=(const metadata_database &in_) {
  auto k_h    = make_handle(*this);
  /// 转换序列化数据
  auto k_data = in_.metadata_cereal().value();
  vector_container my_data{k_data.begin(), k_data.end()};
  {
    vector_istream k_i{my_data};
    boost::archive::text_iarchive k_archive{k_i};
    k_archive >> *this;

    // std::tuple<> k_tu;
    decltype(k_h.try_get<DOODLE_SERIALIZATION>()) k_tu{};
    boost::hana::for_each(k_tu, [&](auto &in_ptr) -> void {
      k_archive >> in_ptr;
    });
    boost::hana::for_each(k_tu, [&](auto &in_ptr) -> void {
      if (in_ptr) {
        k_h.emplace_or_replace<
            std::remove_pointer_t<std::decay_t<decltype(in_ptr)>>>(std::move(*in_ptr));
      }
    });
  }
  /// 转换id
  set_id(in_.id());
  /// 转化类型
  p_type = magic_enum::enum_cast<metadata_type>(in_.m_type().value())
               .value_or(metadata_type::unknown_file);

  /// 确认转换可索引数据

  return *this;
}
database::operator doodle::metadata_database() const {
  auto k_h = make_handle(*this);

  metadata_database k_tmp{};
  ///转换id
  if (p_id != 0)
    k_tmp.set_id(p_id);
  ///设置序列化数据储存位置
  k_tmp.set_uuid_path(get_url_uuid().generic_string());

  /// 设置序列化数据
  vector_container my_data{};
  {
    vector_iostream kt{my_data};
    boost::archive::text_oarchive k_archive{kt};
    k_archive << BOOST_SERIALIZATION_NVP(*this);

    auto k_tu = k_h.try_get<DOODLE_SERIALIZATION>();
    // auto k_boost_tu = boost::hana::to_tuple(k_tu);
    boost::hana::for_each(k_tu, [&](auto &in_ptr) -> void {
      k_archive << in_ptr;
    });
  }
  k_tmp.mutable_metadata_cereal()->set_value(my_data.data(), my_data.size());

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
    k_tmp.mutable_assets()->set_value(k_h.get<assets>().get_name1());
  }

  return k_tmp;
}

#undef DOODLE_SERIALIZATION

bool database::operator==(const database &in_rhs) const {
  return std::tie(p_id, p_uuid) == std::tie(in_rhs.p_id, in_rhs.p_uuid);
}

bool database::operator!=(const database &in_rhs) const {
  return !(*this == in_rhs);
}
bool database::has_child() const {
  return p_has_child > 0;
}
bool database::has_file() const {
  return p_has_file > 0;
}

const std::string &database::get_uuid() const {
  return p_uuid;
}
const string &database::get_id_str() const {
  return p_id_str;
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
