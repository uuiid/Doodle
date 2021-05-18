//
// Created by TD on 2021/5/7.
//

#include <DoodleLib/Metadata/AssetsFile.h>
#include <DoodleLib/Metadata/ContextMenu.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <PinYin/convert.h>

#include <utility>

namespace doodle {

AssetsFile::AssetsFile()
    : Metadata(),
      p_name(),
      p_ShowName(),
      p_time(),
      p_user(),
      p_department(),
      p_comment() {
}
AssetsFile::AssetsFile(std::weak_ptr<Metadata> in_metadata, std::string name, std::string showName)
    : Metadata(),
      p_name(std::move(name)),
      p_ShowName(std::move(showName)),
      p_time(),
      p_user(),
      p_department(),
      p_comment() {
  p_parent = std::move(in_metadata);
  if (p_ShowName.empty())
    p_ShowName = convert::Get().toEn(p_name);
}

std::string AssetsFile::str() const {
  return p_name;
}
std::string AssetsFile::showStr() const {
  return p_ShowName;
}

void AssetsFile::load(const MetadataFactoryPtr& in_factory) {
  in_factory->load(this);
  p_metadata_flctory_ptr_ = in_factory;
}

void AssetsFile::save(const MetadataFactoryPtr& in_factory) {
  p_metadata_flctory_ptr_ = in_factory;
  in_factory->save(this);
}
bool AssetsFile::operator<(const AssetsFile& in_rhs) const {
  return std::tie(p_name, p_ShowName, p_path_file) < std::tie(in_rhs.p_name, in_rhs.p_ShowName, in_rhs.p_path_file);
  //  return std::tie(static_cast<const doodle::Metadata&>(*this), p_name, p_ShowName, p_path_file) < std::tie(static_cast<const doodle::Metadata&>(in_rhs), in_rhs.p_name, in_rhs.p_ShowName, in_rhs.p_path_file);
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
void AssetsFile::modifyParent(const std::shared_ptr<Metadata>& in_old_parent) {
  ///在这里， 如果已经保存过或者已经是从磁盘中加载来时， 必然会持有工厂， 这个时候我们就要告诉工厂， 我们改变了父子关系
  if (p_metadata_flctory_ptr_)
    p_metadata_flctory_ptr_->modifyParent(this, in_old_parent.get());
}
void AssetsFile::createMenu(ContextMenu* in_contextMenu) {
  in_contextMenu->createMenu(std::dynamic_pointer_cast<AssetsFile>(shared_from_this()));
}
const std::chrono::time_point<std::chrono::system_clock>& AssetsFile::getTime() const {
  return p_time;
}
void AssetsFile::setTime(const std::chrono::time_point<std::chrono::system_clock>& in_time) {
  p_time = in_time;
}
const std::string& AssetsFile::getUser() const {
  return p_user;
}
void AssetsFile::setUser(const std::string& in_user) {
  p_user = in_user;
}
const std::string& AssetsFile::getDepartment() const {
  return p_department;
}
void AssetsFile::setDepartment(const std::string& in_department) {
  p_department = in_department;
}
const std::vector<CommentPtr>& AssetsFile::getComment() const {
  return p_comment;
}
void AssetsFile::setComment(const std::vector<CommentPtr>& in_comment) {
  p_comment = in_comment;
}
void AssetsFile::addComment(const CommentPtr& in_comment) {
  p_comment.emplace_back(in_comment);
}
}  // namespace doodle
