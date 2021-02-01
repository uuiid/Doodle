#include <corelib/filesystem/Path.h>
#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>

DOODLE_NAMESPACE_S
Path::Path(const std::string &pathstr)
    : p_path(std::make_shared<fileSys::path>(pathstr)),
      p_exist(false),
      p_isDir(false),
      p_size(0),
      p_time(boost::posix_time::min_date_time) {
}

Path::Path()
    : p_path(std::make_shared<fileSys::path>()),
      p_exist(false),
      p_isDir(false),
      p_size(0),
      p_time(boost::posix_time::min_date_time) {
}

Path::~Path() {
}

const bool &Path::Exist() const noexcept {
  return p_exist;
}

const bool &Path::isDirectory() const noexcept {
  return p_isDir;
}

const uint64_t &Path::size() const noexcept {
  return p_size;
}

const boost::posix_time::ptime &Path::modifyTime() const noexcept {
  return p_time;
}

const std::shared_ptr<fileSys::path> &Path::path() const noexcept {
  return p_path;
}

void Path::setPath(const std::shared_ptr<fileSys::path> &Path_) noexcept {
  p_path = Path_;
}

void to_json(nlohmann::json &j, const Path &p) {
  j = nlohmann::json{
      {"path", p.p_path->generic_string()}};
}

void from_json(const nlohmann::json &j, Path &p) {
  *(p.p_path) = j.at("path").get<std::string>();
  p.p_exist   = j.at("exists").get<bool>();
  p.p_isDir   = j.at("isDirectory").get<bool>();
  p.p_size    = j.at("size").get<int64_t>();
  p.p_time    = boost::posix_time::from_iso_string(
      j.at("modifyTime").get<std::string>());
}
DOODLE_NAMESPACE_E
