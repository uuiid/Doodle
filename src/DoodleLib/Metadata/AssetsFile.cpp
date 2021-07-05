//
// Created by TD on 2021/5/7.
//

#include <DoodleLib/Metadata/AssetsFile.h>
#include <DoodleLib/Metadata/AssetsPath.h>
#include <core/CoreSet.h>
///这个工厂类必须在所有导入的后面
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <Gui/factory/menu_factory.h>
#include <Metadata/TimeDuration.h>
#include <PinYin/convert.h>

#include <utility>

namespace doodle {

AssetsFile::AssetsFile()
    : Metadata(),
      p_name(),
      p_ShowName(),
      p_path_file(),
      p_path_files(),
      p_time(std::make_shared<TimeDuration>(std::chrono::system_clock::now())),
      p_user(CoreSet::getSet().getUser()),
      p_department(CoreSet::getSet().getDepartmentEnum()),
      p_comment(),
      p_version(1) {
}

AssetsFile::AssetsFile(std::weak_ptr<Metadata> in_metadata, std::string showName, std::string name)
    : Metadata(),
      p_name(std::move(name)),
      p_ShowName(std::move(showName)),
      p_path_file(std::make_shared<AssetsPath>()),
      p_path_files(),
      p_time(std::make_shared<TimeDuration>(std::chrono::system_clock::now())),
      p_user(CoreSet::getSet().getUser()),
      p_department(CoreSet::getSet().getDepartmentEnum()),
      p_comment(),
      p_version(1) {
  p_parent = std::move(in_metadata);
  if (p_name.empty())
    p_name = convert::Get().toEn(p_ShowName);
}

// AssetsFile::~AssetsFile() {
//   if (p_metadata_flctory_ptr_)
//     updata_db(p_metadata_flctory_ptr_);
// }

std::string AssetsFile::str() const {
  return p_name;
}
std::string AssetsFile::showStr() const {
  return p_ShowName;
}

bool AssetsFile::operator<(const AssetsFile& in_rhs) const {
  return std::tie(p_name, p_ShowName, p_version) < std::tie(in_rhs.p_name, in_rhs.p_ShowName, p_version);
  //  return std::tie(static_cast<const doodle::Metadata&>(*this), p_name, p_ShowName) < std::tie(static_cast<const doodle::Metadata&>(in_rhs), in_rhs.p_name, in_rhs.p_ShowName);
}
bool AssetsFile::operator>(const AssetsFile& in_rhs) const {
  return in_rhs < *this;
}
bool AssetsFile::operator<=(const AssetsFile& in_rhs) const {
  return !(in_rhs < *this);
}
bool AssetsFile::operator>=(const AssetsFile& in_rhs) const {
  return !(*this < in_rhs);
}

bool AssetsFile::sort(const Metadata& in_rhs) const {
  if (typeid(in_rhs) == typeid(*this)) {
    return *this < (dynamic_cast<const AssetsFile&>(in_rhs));
  } else {
    return str() < in_rhs.str();
  }
}
std::chrono::time_point<std::chrono::system_clock> AssetsFile::getStdTime() const {
  return p_time->getUTCTime();
}
void AssetsFile::setStdTime(const std::chrono::time_point<std::chrono::system_clock>& in_time) {
  p_time = std::make_shared<TimeDuration>(in_time);
  saved(true);
  sig_change();
}
const std::string& AssetsFile::getUser() const {
  return p_user;
}
void AssetsFile::setUser(const std::string& in_user) {
  p_user = in_user;
  saved(true);
  sig_change();
}

const std::vector<CommentPtr>& AssetsFile::getComment() const {
  return p_comment;
}
void AssetsFile::setComment(const std::vector<CommentPtr>& in_comment) {
  p_comment = in_comment;
  saved(true);
  sig_change();
}
void AssetsFile::addComment(const CommentPtr& in_comment) {
  p_comment.emplace_back(in_comment);
  saved(true);
  sig_change();
}

const std::uint64_t& AssetsFile::getVersion() const noexcept {
  return p_version;
}

std::string AssetsFile::getVersionStr() const {
  return fmt::format("v{:04d}", p_version);
}

void AssetsFile::setVersion(const std::uint64_t& in_Version) noexcept {
  p_version = in_Version;
}
const std::vector<AssetsPathPtr>& AssetsFile::getPathFile() const {
  return p_path_files;
}
void AssetsFile::setPathFile(const std::vector<AssetsPathPtr>& in_pathFile) {
  p_path_files = in_pathFile;
  saved(true);
  sig_change();
}
Department AssetsFile::getDepartment() const {
  return p_department;
}
void AssetsFile::setDepartment(Department in_department) {
  p_department = in_department;
  saved(true);
  sig_change();
}
void AssetsFile::_select_indb(const MetadataFactoryPtr& in_factory) {
  p_metadata_flctory_ptr_->select_indb(this);
}

void AssetsFile::_updata_db(const MetadataFactoryPtr& in_factory) {
  if (isInstall())
    p_metadata_flctory_ptr_->updata_db(this);
  else
    p_metadata_flctory_ptr_->insert_into(this);
}
void AssetsFile::_deleteData(const MetadataFactoryPtr& in_factory) {
  in_factory->deleteData(this);
}
void AssetsFile::_insert_into(const MetadataFactoryPtr& in_factory) {
  in_factory->insert_into(this);
}
const TimeDurationPtr& AssetsFile::getTime() const {
  return p_time;
}
void AssetsFile::setTime(const TimeDurationPtr& in_time) {
  p_time = in_time;
  saved(true);
  sig_change();
}
void AssetsFile::create_menu(const menu_factory_ptr& in_factoryPtr) {
  in_factoryPtr->create_menu(std::dynamic_pointer_cast<AssetsFile>(shared_from_this()));
}
}  // namespace doodle
