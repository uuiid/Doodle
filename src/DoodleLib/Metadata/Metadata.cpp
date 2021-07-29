//
// Created by teXiao on 2021/4/27.
//

#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/Metadata/Metadata.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <Exception/Exception.h>
#include <Metadata/Metadata_cpp.h>
#include <core/ContainerDevice.h>
#include <core/CoreSet.h>
#include <google/protobuf/util/time_util.h>

#include <cereal/archives/portable_binary.hpp>

namespace doodle {
Metadata::Metadata()
    : std::enable_shared_from_this<Metadata>(),
      p_parent(),
      p_id(0),
      p_parent_id(),
      p_uuid(std::move(CoreSet::getSet().getUUIDStr())),
      p_metadata_flctory_ptr_(),
      p_need_save(true),
      p_need_load(true),
      p_updata_parent_id(false),
      p_has_child(0),
      child_item(),
      user_date(),
      p_type(meta_type::unknown_file),
      p_updata_type(false),
      child_item_is_sort(false) {
  install_slots();
}

Metadata::Metadata(std::weak_ptr<Metadata> in_metadata)
    : std::enable_shared_from_this<Metadata>(),
      p_parent(std::move(in_metadata)),
      p_id(0),
      p_parent_id(p_parent.lock()->p_id),
      p_uuid(std::move(CoreSet::getSet().getUUIDStr())),
      p_metadata_flctory_ptr_(),
      p_need_save(true),
      p_need_load(true),
      p_updata_parent_id(false),
      p_has_child(0),
      child_item(),
      user_date(),
      p_type(meta_type::unknown_file),
      p_updata_type(false),
      child_item_is_sort(false) {
  install_slots();
}

Metadata::~Metadata() = default;

std::shared_ptr<Metadata> Metadata::getParent() const {
  return p_parent.lock();
}

void Metadata::sortChildItems(bool is_launch_sig) {
  if (child_item_is_sort)
    return;

  if (is_launch_sig)
    child_item.sig_begin_sort(child_item);
  std::sort(child_item.begin(), child_item.end(),
            [](const MetadataPtr &r, const MetadataPtr &l) {
              return *r < *l;
            });
  if (is_launch_sig)
    child_item.sig_sort(child_item);

  child_item_is_sort = true;
}

bool Metadata::hasParent() const {
  return !p_parent.expired();
}
bool Metadata::hasChild() const {
  return p_has_child > 0;
}
std::string Metadata::showStr() const {
  return str();
}
const std::string &Metadata::getUUID() const {
  return p_uuid;
}

const MetadataFactoryPtr &Metadata::getMetadataFactory() const {
  return p_metadata_flctory_ptr_;
}
bool Metadata::checkParent(const Metadata &in_metadata) const {
  return p_parent_id == in_metadata.p_id;
}

bool Metadata::operator<(const Metadata &in_rhs) const {
  return this->sort(in_rhs);
}
bool Metadata::operator>(const Metadata &in_rhs) const {
  return in_rhs < *this;
}
bool Metadata::operator<=(const Metadata &in_rhs) const {
  return !(in_rhs < *this);
}
bool Metadata::operator>=(const Metadata &in_rhs) const {
  return !(*this < in_rhs);
}
MetadataConstPtr Metadata::getRootParent() const {
  auto k_p = shared_from_this();
  while (!k_p->p_parent.expired()) {
    k_p = k_p->p_parent.lock()->getRootParent();
  }
  return k_p;
  //  if(p_parent.expired())
  //    return shared_from_this();
  //  else
  //    return p_parent.lock()->getRootParent();
}

void Metadata::loaded(bool in_need) {
  p_need_load = in_need;
}
void Metadata::saved(bool in_need) {
  if (!in_need)
    p_updata_parent_id = false;
  p_need_save = in_need;
}
bool Metadata::isLoaded() const {
  return !p_need_load;
}
bool Metadata::isSaved() const {
  return !p_need_save;
}
FSys::path Metadata::getUrlUUID() const {
  auto name = FSys::path{getRootParent()->getUUID()};
  name /= p_uuid.substr(0, 3);
  name /= p_uuid;
  return name;
}
uint64_t Metadata::getId() const {
  return p_id;
}

void Metadata::set_meta_typp(const meta_type &in_meta) {
  p_type        = in_meta;
  p_updata_type = true;
}

void Metadata::set_meta_typp(const std::string &in_meta) {
  p_type        = magic_enum::enum_cast<meta_type>(in_meta).value_or(meta_type::unknown_file);
  p_updata_type = true;
}

void Metadata::set_meta_type(std::int32_t in_) {
  p_type        = magic_enum::enum_cast<meta_type>(in_).value_or(meta_type::unknown_file);
  p_updata_type = true;
}

Metadata::meta_type Metadata::get_meta_type() const {
  return p_type;
}

std::string Metadata::get_meta_type_str() const {
  return std::string{magic_enum::enum_name(p_type)};
}

std::int32_t Metadata::get_meta_type_int() const {
  return magic_enum::enum_integer(p_type);
}
bool Metadata::operator==(const Metadata &in_rhs) const {
  return std::tie(p_id) == std::tie(in_rhs.p_id);
}
bool Metadata::operator!=(const Metadata &in_rhs) const {
  return !(in_rhs == *this);
}

void Metadata::select_indb(const MetadataFactoryPtr &in_factory) {
  if (in_factory)
    p_metadata_flctory_ptr_ = in_factory;
  if (isLoaded())
    return;

  _select_indb(p_metadata_flctory_ptr_);
  loaded();
}

void Metadata::updata_db(const MetadataFactoryPtr &in_factory) {
  if (in_factory)
    p_metadata_flctory_ptr_ = in_factory;

  if (isSaved())
    return;

  ///在这里测试使用具有父级， 并且如果有父级， 还要更新父id， 那么就可以断定也要更新父级的记录
  if (hasParent() && p_metadata_flctory_ptr_) {
    p_parent.lock()->updata_db(p_metadata_flctory_ptr_);
  }

  if (isInstall())
    _updata_db(p_metadata_flctory_ptr_);
  else
    _insert_into(p_metadata_flctory_ptr_);

  saved();
}

void Metadata::deleteData(const MetadataFactoryPtr &in_factory) {
  if (in_factory)
    p_metadata_flctory_ptr_ = in_factory;

  _deleteData(p_metadata_flctory_ptr_);
}

void Metadata::insert_into(const MetadataFactoryPtr &in_factory) {
  if (in_factory)
    p_metadata_flctory_ptr_ = in_factory;

  _insert_into(p_metadata_flctory_ptr_);
  saved();
}
void Metadata::install_slots() {
  child_item.sig_begin_clear.connect([this]() {
    for (const auto &k_i : this->child_item) {
      k_i->p_id = 0;
    }
    p_has_child = 0;
    saved(true);
  });

  child_item.sig_begin_insert.connect([this](const MetadataPtr &val) {
    add_child(val);
    ++p_has_child;
    saved(true);
  });
  child_item.sig_begin_erase.connect([this](const MetadataPtr &val) {
    --p_has_child;
    saved(true);
  });

  child_item.sig_begin_push_back.connect([this](const MetadataPtr &val) {
    add_child(val);
    ++p_has_child;
    saved(true);
  });

  child_item.sig_begin_swap.connect([this](const std::vector<MetadataPtr> &val) {
    for (auto &k_i : val) {
      add_child(k_i);
    }
    p_has_child = val.size();
    saved(true);
  });
}
void Metadata::add_child(const MetadataPtr &val) {
  /// 先查看是否有父级关联
  if (val->hasParent()) {
    /// 有关联并且父物体不是指向自己的话
    /// 那么我们要同时记录子项要更新父id属性和要保存所有属性
    if (val->p_parent.lock() != shared_from_this()) {
      /// 设置保存指向父的id需求
      val->p_updata_parent_id = true;
      /// 设置为需求保存
      val->saved(true);
      /// 在父物体上调用清除子物体信号
      val->p_parent.lock()->child_item.erase_sig(val);
    }
  }

  /// 这里将所有的子级要继承的父级属性给上
  val->p_parent                = weak_from_this();
  val->p_parent_id             = p_id;
  val->p_metadata_flctory_ptr_ = p_metadata_flctory_ptr_;
  child_item_is_sort           = false;

  DOODLE_LOG_INFO(fmt::format("插入子数据： {}", val->showStr()))
}
Metadata::operator DataDb() const {
  DataDb k_tmp{};
  this->to_DataDb(k_tmp);
  return k_tmp;
}

void Metadata::to_DataDb(DataDb &in_) const {
  in_.set_id(p_id);
  in_.set_uuidpath(getUrlUUID().generic_string());
  if (hasParent() && (p_updata_parent_id || p_id == 0))
    in_.mutable_parent()->set_value(*p_parent_id);

  //  auto k_time      = std::chrono::system_clock::now();
  //  auto k_timestamp = google::protobuf::util::TimeUtil::TimeTToTimestamp(
  //      std::chrono::system_clock::to_time_t(k_time));
  //  in_.mutable_update_time()->CopyFrom(k_timestamp);
  if (p_id != 0) {
    vector_container my_data{};
    {
      vector_iostream kt{my_data};
      cereal::PortableBinaryOutputArchive k_archive{kt};
      k_archive(shared_from_this());
    }
    in_.mutable_metadata_cereal()->set_value(my_data.data(), my_data.size());
  }

  if (p_updata_type || p_id == 0)
    in_.mutable_m_type()->set_value(magic_enum::enum_cast<doodle::DataDb::meta_type>(get_meta_type_int()).value());
}
MetadataPtr Metadata::from_DataDb(const DataDb &in_) {
  MetadataPtr k_ptr{};

  auto k_data = in_.metadata_cereal().value();
  vector_container my_data{k_data.begin(), k_data.end()};
  {
    vector_istream k_i{my_data};
    cereal::PortableBinaryInputArchive k_archive{k_i};
    k_archive(k_ptr);
  }

  if (k_ptr->p_id != in_.id())
    throw DoodleError{fmt::format("验证出错 id 不相同 {} == {}", k_ptr->p_id, in_.id())};

  if (in_.parent().value() != 0) {  ///  不为零的情况下, 比较验证值, 不相同就返回空指针
    if (k_ptr->p_parent_id != in_.parent().value())
      throw DoodleError{fmt::format("验证出错, 父id不相同 {} == {}", k_ptr->p_parent_id.value(), in_.parent().value())};
  } else {
    if (k_ptr->p_parent_id)  /// 是零就是默认值, 没有值, 如果父id有值就直接返回空
      throw DoodleError{fmt::format("验证出错, 没有父id, 但是传入父id {} ", in_.parent().value())};
  }

  k_ptr->set_meta_type(magic_enum::enum_integer(in_.m_type().value()));
  return k_ptr;
}
std::uint64_t Metadata::get_parent_id() const {
  return *p_parent_id;
}
bool Metadata::has_parent_id() const {
  return p_parent_id.has_value();
}

}  // namespace doodle
