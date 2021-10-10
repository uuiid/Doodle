//
// Created by TD on 2021/5/7.
//

#include <doodle_lib/metadata/assets_file.h>
#include <doodle_lib/metadata/assets_path.h>
#include <doodle_lib/metadata/comment.h>
///这个工厂类必须在所有导入的后面
#include <doodle_lib/gui/factory/attribute_factory_interface.h>
#include <doodle_lib/metadata/metadata_factory.h>
#include <metadata/time_point_wrap.h>
#include <pin_yin/convert.h>
#include <google/protobuf/util/time_util.h>


BOOST_CLASS_EXPORT_IMPLEMENT(doodle::assets_file)
namespace doodle {
void assets_file::serialize_check() {
  if (!p_path_files)
    p_path_files = new_object<assets_path_vector>();
  if (!p_comment)
    p_comment = new_object<comment_vector>();
}

assets_file::assets_file()
    : metadata(),
      p_name(),
      p_ShowName(),
      p_path_files(new_object<assets_path_vector>()),
      p_time(new_object<time_point_wrap>(std::chrono::system_clock::now())),
      p_user(core_set::getSet().get_user()),
      p_department(core_set::getSet().get_department_enum()),
      p_comment(new_object<comment_vector>()),
      p_version(1),
      p_need_time(false) {
  p_type = meta_type::file;
}

assets_file::assets_file(std::weak_ptr<metadata> in_metadata, std::string showName, std::string name)
    : metadata(in_metadata),
      p_name(std::move(name)),
      p_ShowName(std::move(showName)),
      p_path_files(new_object<assets_path_vector>()),
      p_time(new_object<time_point_wrap>(std::chrono::system_clock::now())),
      p_user(core_set::getSet().get_user()),
      p_department(core_set::getSet().get_department_enum()),
      p_comment(new_object<comment_vector>()),
      p_version(1),
      p_need_time(false) {
  p_type   = meta_type::file;
  p_parent = std::move(in_metadata);
  if (p_name.empty())
    p_name = convert::Get().toEn(p_ShowName);
}

// AssetsFile::~AssetsFile() {
//   if (p_metadata_flctory_ptr_)
//     updata_db(p_metadata_flctory_ptr_);
// }

std::string assets_file::str() const {
  return p_name;
}
std::string assets_file::show_str() const {
  return p_ShowName;
}

bool assets_file::operator<(const assets_file& in_rhs) const {
  // return std::tie(p_version, p_time->getUTCTime()) < std::tie(p_version, p_time->getUTCTime());
  return std::tie(p_version) < std::tie(in_rhs.p_version);
  //  return std::tie(static_cast<const doodle::metadata&>(*this), p_name, p_ShowName) < std::tie(static_cast<const doodle::metadata&>(in_rhs), in_rhs.p_name, in_rhs.p_ShowName);
}
bool assets_file::operator>(const assets_file& in_rhs) const {
  return in_rhs < *this;
}
bool assets_file::operator<=(const assets_file& in_rhs) const {
  return !(in_rhs < *this);
}
bool assets_file::operator>=(const assets_file& in_rhs) const {
  return !(*this < in_rhs);
}

const std::string& assets_file::get_user() const {
  return p_user;
}
void assets_file::set_user(const std::string& in_user) {
  p_user = in_user;
  saved(true);
}

const std::uint64_t& assets_file::get_version() const noexcept {
  return p_version;
}

std::string assets_file::get_version_str() const {
  return fmt::format("v{:04d}", p_version);
}

void assets_file::set_version(const std::uint64_t& in_Version) noexcept {
  p_version = in_Version;
}

int assets_file::find_max_version() const {
  if (p_parent.expired())
    return 1;
  auto k_p = p_parent.lock();

  if (k_p->get_child().empty())
    return 1;
  std::vector<metadata_ptr> k_r;
  std::vector<assets_file_ptr> k_assetsFilePtr;

  std::copy_if(
      k_p->get_child().begin(), k_p->get_child().end(),
      std::inserter(k_r, k_r.begin()), [this](const metadata_ptr& in_) {
        if (details::is_class<assets_file>(in_)) {
          return std::dynamic_pointer_cast<assets_file>(in_)->get_department() == get_department();
        } else
          return false;
      });
  std::transform(k_r.begin(), k_r.end(), std::back_inserter(k_assetsFilePtr),
                 [](const metadata_ptr& in_) {
                   return std::dynamic_pointer_cast<assets_file>(in_);
                 });

  std::size_t k_int{0};
  std::sort(k_assetsFilePtr.begin(), k_assetsFilePtr.end(), [](const assets_file_ptr& in_a, const assets_file_ptr& in_b) {
    return *in_a < *in_b;
  });
  if (!k_assetsFilePtr.empty())
    k_int = k_assetsFilePtr.back()->get_version() + 1;
  else
    k_int = 1;
  return boost::numeric_cast<std::int32_t>(k_int);
}

department assets_file::get_department() const {
  return p_department;
}
void assets_file::set_department(department in_department) {
  p_department = in_department;
  saved(true);
}
const time_wrap_ptr& assets_file::get_time() {
  p_time->set_metadata(shared_from_this());
  return p_time;
}
void assets_file::set_time(const time_wrap_ptr& in_time) {
  p_time = in_time;
  saved(true);
  p_need_time = true;
}
void assets_file::attribute_widget(const attribute_factory_ptr& in_factoryPtr) {
  in_factoryPtr->show_attribute(std::dynamic_pointer_cast<assets_file>(shared_from_this()));
}
void assets_file::to_DataDb(metadata_database& in_) const {
  metadata::to_DataDb(in_);
  if (p_need_time || p_id == 0) {
    auto k_timestamp = google::protobuf::util::TimeUtil::TimeTToTimestamp(
        p_time->get_local_time_t());
    in_.mutable_update_time()->CopyFrom(k_timestamp);
  }
}
void assets_file::set_path_file(const assets_path_vector_ptr& in_) {
  p_path_files = in_;
}
void assets_file::set_comment(const comment_vector_ptr& in_) {
  p_comment = in_;
}
assets_path_vector_ptr assets_file::get_path_file() {
  if (p_path_files)
    p_path_files->set_metadata(shared_from_this());
  return p_path_files;
}
comment_vector_ptr assets_file::get_comment() {
  if (p_comment)
    p_comment->set_metadata(shared_from_this());
  return p_comment;
}

}  // namespace doodle
