//
// Created by teXiao on 2021/4/27.
//

#include "metadata.h"

#include <Exception/exception.h>
#include <Metadata/metadata_cpp.h>
#include <core/ContainerDevice.h>
#include <core/core_set.h>
#include <doodle_lib/Logger/logger.h>
#include <doodle_lib/metadata/metadata_factory.h>
#include <google/protobuf/util/time_util.h>

#include <boost/algorithm/algorithm.hpp>
#include <boost/archive/polymorphic_text_iarchive.hpp>
#include <boost/archive/polymorphic_text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/range/algorithm/count_if.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(doodle::metadata)
namespace doodle {
metadata::metadata()
    : database_action<metadata, metadata_factory>(this),
      std::enable_shared_from_this<metadata>(),
      p_parent(),
      p_parent_id(),
      p_uuid(std::move(core_set::getSet().get_uuid_str())),
      p_updata_parent_id(false),
      p_has_child(0),
      p_has_file(0),
      child_item(),
      p_type(meta_type::unknown_file),
      p_updata_type(false),
      child_item_is_sort(false) {
}

metadata::metadata(std::weak_ptr<metadata> in_metadata)
    : database_action<metadata, metadata_factory>(this),
      std::enable_shared_from_this<metadata>(),
      p_parent(std::move(in_metadata)),
      p_parent_id(p_parent.lock()->p_id),
      p_uuid(std::move(core_set::getSet().get_uuid_str())),
      p_updata_parent_id(false),
      p_has_child(0),
      p_has_file(0),
      child_item(),
      p_type(meta_type::unknown_file),
      p_updata_type(false),
      child_item_is_sort(false) {
}

metadata::~metadata() = default;

std::shared_ptr<metadata> metadata::get_parent() const {
  return p_parent.lock();
}

void metadata::sort_child_items(bool is_launch_sig) {
  //  std::sort(child_item.begin(), child_item.end(),
  //            [](const metadata_ptr &r, const metadata_ptr &l) {
  //              return *r < *l;
  //            });
}

bool metadata::has_parent() const {
  return !p_parent.expired();
}
bool metadata::has_child() const {
  return p_has_child > 0;
}

bool metadata::has_file() const {
  return p_has_file > 0;
}
std::string metadata::show_str() const {
  return str();
}
const std::string &metadata::get_uuid() const {
  return p_uuid;
}

const metadata_factory_ptr &metadata::get_metadata_factory() const {
  return p_factory;
}
bool metadata::check_parent(const metadata &in_metadata) const {
  return p_parent_id == in_metadata.p_id;
}

bool metadata::operator<(const metadata &in_rhs) const {
  return str() < str();
}
bool metadata::operator>(const metadata &in_rhs) const {
  return in_rhs < *this;
}
bool metadata::operator<=(const metadata &in_rhs) const {
  return !(in_rhs < *this);
}
bool metadata::operator>=(const metadata &in_rhs) const {
  return !(*this < in_rhs);
}
metadata_const_ptr metadata::get_root_parent() const {
  auto k_p = shared_from_this();
  while (!k_p->p_parent.expired()) {
    k_p = k_p->p_parent.lock()->get_root_parent();
  }
  return k_p;
  //  if(p_parent.expired())
  //    return shared_from_this();
  //  else
  //    return p_parent.lock()->getRootParent();
}

FSys::path metadata::get_url_uuid() const {
  auto name = FSys::path{get_root_parent()->get_uuid()};
  name /= p_uuid.substr(0, 3);
  name /= p_uuid;
  return name;
}

void metadata::set_meta_typp(const meta_type &in_meta) {
  p_type        = in_meta;
  p_updata_type = true;
}

void metadata::set_meta_typp(const std::string &in_meta) {
  p_type        = magic_enum::enum_cast<meta_type>(in_meta).value_or(meta_type::unknown_file);
  p_updata_type = true;
}

void metadata::set_meta_type(std::int32_t in_) {
  p_type        = magic_enum::enum_cast<meta_type>(in_).value_or(meta_type::unknown_file);
  p_updata_type = true;
}

metadata::meta_type metadata::get_meta_type() const {
  return p_type;
}

std::string metadata::get_meta_type_str() const {
  return std::string{magic_enum::enum_name(p_type)};
}

std::int32_t metadata::get_meta_type_int() const {
  return magic_enum::enum_integer(p_type);
}
bool metadata::operator==(const metadata &in_rhs) const {
  return std::tie(p_id) == std::tie(in_rhs.p_id);
}
bool metadata::operator!=(const metadata &in_rhs) const {
  return std::tie(p_id) != std::tie(in_rhs.p_id);
}

void metadata::end_push_back(const metadata_ptr &in_val) {
  add_child(in_val);
  switch (in_val->p_type) {
    case meta_type::unknown_file:
    case meta_type::project_root:
    case meta_type::animation_lib_root:
      break;
    case meta_type::folder:
      ++p_has_child;
      break;
    case meta_type::derive_file:
    case meta_type::file: {
      ++p_has_file;
      break;
    }
    default:
      break;
  }

  saved(true);
}

void metadata::end_erase(const metadata_ptr &in_val) {
  switch (in_val->p_type) {
    case meta_type::unknown_file:
    case meta_type::project_root:
    case meta_type::animation_lib_root:
      break;
    case meta_type::folder:
      --p_has_child;
      break;
    case meta_type::derive_file:
    case meta_type::file: {
      --p_has_file;
      break;
    }
    default:
      break;
  }
  saved(true);
}

void metadata::end_clear() {
  for (const auto &k_i : this->child_item) {
    k_i->p_id = 0;
  }
  p_has_child = 0;
  saved(true);
}
void metadata::add_child(const metadata_ptr &val) {
  /// 先查看是否有父级关联
  if (val->has_parent()) {
    /// 有关联并且父物体不是指向自己的话
    /// 那么我们要同时记录子项要更新父id属性和要保存所有属性
    if (val->p_parent.lock() != shared_from_this()) {
      /// 设置保存指向父的id需求
      val->p_updata_parent_id = true;
      /// 设置为需求保存
      val->saved(true);
      /// 在父物体上调用清除子物体信号
      val->p_parent.lock()->get_child().erase(val);
    }
  }

  /// 这里将所有的子级要继承的父级属性给上
  val->p_parent      = weak_from_this();
  val->p_parent_id   = p_id;
  val->p_factory     = p_factory;
  child_item_is_sort = false;

  DOODLE_LOG_INFO(fmt::format("插入子数据： {}", val->show_str()))
}
metadata::operator metadata_database() const {
  metadata_database k_tmp{};
  this->to_DataDb(k_tmp);
  return k_tmp;
}

void metadata::to_DataDb(metadata_database &in_) const {
  in_.set_id(p_id);
  in_.set_uuid_path(get_url_uuid().generic_string());
  if (has_parent() && (p_updata_parent_id || p_id == 0))
    in_.mutable_parent()->set_value(*p_parent_id);

  //  auto k_time      = std::chrono::system_clock::now();
  //  auto k_timestamp = google::protobuf::util::TimeUtil::TimeTToTimestamp(
  //      std::chrono::system_clock::to_time_t(k_time));
  //  in_.mutable_update_time()->CopyFrom(k_timestamp);
  if (p_id != 0) {
    vector_container my_data{};
    {
      vector_iostream kt{my_data};
      boost::archive::text_oarchive k_archive{kt};
      auto k_ptr = shared_from_this();
      k_archive << boost::serialization::make_nvp("meta", k_ptr);
    }
    in_.mutable_metadata_cereal()->set_value(my_data.data(), my_data.size());
  }

  if (p_updata_type || p_id == 0)
    in_.mutable_m_type()->set_value(magic_enum::enum_cast<doodle::metadata_database::meta_type>(get_meta_type_int()).value());
}
metadata_ptr metadata::from_DataDb(const metadata_database &in_) {
  metadata_ptr k_ptr{};
  try {
    auto k_data = in_.metadata_cereal().value();
    vector_container my_data{k_data.begin(), k_data.end()};
    {
      vector_istream k_i{my_data};
      boost::archive::text_iarchive k_archive{k_i};
      k_archive >> k_ptr;
    }

    if (k_ptr->p_id != in_.id())
      throw doodle_error{fmt::format("验证出错 id 不相同 {} == {}", k_ptr->p_id, in_.id())};

    if (in_.parent().value() != 0) {  ///  不为零的情况下, 比较验证值, 不相同就返回空指针
      if (k_ptr->p_parent_id != in_.parent().value())
        throw doodle_error{fmt::format("验证出错, 父id不相同 {} == {}", k_ptr->p_parent_id.value(), in_.parent().value())};
    } else {
      if (k_ptr->p_parent_id)  /// 是零就是默认值, 没有值, 如果父id有值就直接返回空
        throw doodle_error{fmt::format("验证出错, 没有父id, 但是传入父id {} ", in_.parent().value())};
    }

    k_ptr->set_meta_type(magic_enum::enum_integer(in_.m_type().value()));
  } catch (boost::archive::archive_exception &err) {
    DOODLE_LOG_WARN(err.what());
  }

  return k_ptr;
}
std::uint64_t metadata::get_parent_id() const {
  return *p_parent_id;
}
bool metadata::has_parent_id() const {
  return p_parent_id.has_value();
}

const entt::entity &tree_relationship::get_parent() const noexcept {
  return p_parent;
}
void tree_relationship::set_parent(const entt::entity &in_parent) noexcept {
  p_parent = in_parent;
}

const std::vector<entt::entity> &tree_relationship::get_child() const noexcept {
  return p_child;
}

std::vector<entt::entity> &tree_relationship::get_child() noexcept {
  return p_child;
}

void tree_relationship::set_child(const std::vector<entt::entity> &in_child) noexcept {
  p_child = in_child;
}

entt::entity tree_relationship::get_root() const {
  auto &reg = core_set::getSet().reg;
  const tree_relationship *k_t{this};
  entt::entity k_parent{p_parent};
  while (k_parent != entt::null) {
    k_t      = reg.try_get<tree_relationship>(k_parent);
    k_parent = k_t->p_parent;
  }
  return k_parent;
  // if (k_t->has_parent())
  //   return reg.get<tree_relationship>(p_parent);
  // else {
  //   return entt::to_entity(reg, *this);
  // }
}

database::database()
    : p_id(0),
      p_parent_id(),
      p_type(metadata::meta_type::unknown_file),
      p_uuid(core_set::getSet().get_uuid_str()),
      p_has_child(0),
      p_has_file(0),
      p_updata_parent_id(false),
      p_updata_type(false),
      p_need_load(false),
      p_need_save(false),
      p_boost_serialize_vesion(0) {
}

FSys::path database::get_url_uuid() const {
  auto &l_reg  = core_set::getSet().reg;
  auto l_ent   = entt::to_entity(l_reg, *this);
  // 找到本身的树类
  auto &l_tree = l_reg.get<tree_relationship>(l_ent);
  // 找到根的数据库类
  auto &k_data = l_reg.get<database>(l_tree.get_root());

  // 组合路径
  auto path    = FSys::path{k_data.p_uuid};
  path /= p_uuid.substr(0, 3);
  path /= p_uuid;
  return path;
}

bool database::has_parent() const {
  return p_parent_id.has_value();
}

void database::set_meta_typp(const metadata::meta_type &in_meta) {
  p_type        = in_meta;
  p_updata_type = true;
};

void database::set_meta_typp(const std::string &in_meta) {
  p_type = magic_enum::enum_cast<metadata::meta_type>(in_meta)
               .value_or(metadata::meta_type::unknown_file);
  p_updata_type = true;
};

void database::set_meta_type(std::int32_t in_) {
  p_type = magic_enum::enum_cast<metadata::meta_type>(in_)
               .value_or(metadata::meta_type::unknown_file);
  p_updata_type = true;
};

std::int32_t database::get_meta_type_int() const {
  return magic_enum::enum_integer(p_type);
}

database &database::operator=(const metadata_database &in_) {
  auto k_data = in_.metadata_cereal().value();
  vector_container my_data{k_data.begin(), k_data.end()};
  {
    auto &l_reg = core_set::getSet().reg;
    auto l_m    = entt::meta<project>();
    auto l_ent  = entt::to_entity(l_reg, *this);
    vector_istream k_i{my_data};
    boost::archive::text_iarchive k_archive{k_i};
    k_archive >> *this;

    // std::tuple<> k_tu;
    decltype(l_reg.try_get<project, episodes, shot, season, assets, assets_file>(l_ent)) k_tu{};
    boost::hana::for_each(k_tu, [&](auto &in_ptr) -> void {
      k_archive >> in_ptr;
    });
    boost::hana::for_each(k_tu, [&](auto &in_ptr) -> void {
      if (in_ptr) {
        l_reg.emplace_or_replace<decltype(*in_ptr)>(l_ent, *in_ptr);
      }
    });
  }
  p_type = magic_enum::enum_cast<metadata::meta_type>(
               magic_enum::enum_integer(in_.m_type().value()))
               .value_or(metadata::meta_type::unknown_file);
}
database::operator doodle::metadata_database() const {
  metadata_database k_tmp{};
  k_tmp.set_id(p_id);
  k_tmp.set_uuid_path(get_url_uuid().generic_string());
  if (has_parent() && (p_updata_parent_id || p_id == 0))
    k_tmp.mutable_parent()->set_value(*p_parent_id);

  vector_container my_data{};
  {
    auto &l_reg = core_set::getSet().reg;
    auto l_m    = entt::meta<project>();
    auto l_ent  = entt::to_entity(l_reg, *this);
    vector_iostream kt{my_data};
    boost::archive::text_oarchive k_archive{kt};
    k_archive << BOOST_SERIALIZATION_NVP(*this);

    auto &k_tu = l_reg.try_get<project, episodes, shot, season, assets, assets_file>(l_ent);
    boost::hana::for_each(k_tu, [&](auto &in_ptr) -> void {
      k_archive << in_ptr;
    });
  }
  k_tmp.mutable_metadata_cereal()->set_value(my_data.data(), my_data.size());

  if (p_updata_type || p_id == 0) {
    k_tmp.mutable_m_type()->set_value(
        magic_enum::enum_cast<doodle::metadata_database::meta_type>(get_meta_type_int()).value());
  }
  return k_tmp;
}

FSys::path database::get_url_uuid() const {
  auto &l_reg  = core_set::getSet().reg;
  auto l_ent   = entt::to_entity(l_reg, *this);
  // 找到本身的树类
  auto &l_tree = l_reg.get<tree_relationship>(l_ent);
  // 找到根的数据库类
  auto &k_data = l_reg.get<database>(l_tree.get_root());

  // 组合路径
  auto path    = FSys::path{k_data.p_uuid};
  path /= p_uuid.substr(0, 3);
  path /= p_uuid;
  return path;
}

bool database::has_parent() const {
  return p_parent_id.has_value();
}
}  // namespace doodle
