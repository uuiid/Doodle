#include "path.h"
#include <src/Project.h>

#include <src/FileSystem.h>
#include <boost/filesystem.hpp>
DOODLE_NAMESPACE_S
Path::Path(std::string& str)
    : p_path(nullptr),
      p_prj_path(nullptr),
      p_exist(false),
      p_isDir(false),
      p_size(0),
      p_time(boost::posix_time::min_date_time) {
  p_path = std::make_shared<boost::filesystem::path>(str);
  p_path = std::make_shared<boost::filesystem::path>("");
  scanningInfo();
}

Path::Path()
    : p_path(std::make_shared<boost::filesystem::path>("")),
      p_prj_path(std::make_shared<boost::filesystem::path>("")),
      p_exist(false),
      p_isDir(false),
      p_size(0),
      p_time(boost::posix_time::min_date_time),
      p_file() {
}

Path::~Path() {
}

bool Path::exists() const {
  return p_exist;
}

bool Path::isDirectory() const {
  return p_isDir;
}

bool Path::createFolder() const {
  return boost::filesystem::create_directories(*p_prj_path / *p_path);
}

bool Path::read(char* buffer, uint64_t size, uint64_t offset) {
  if (!p_file) {
    p_file = FileSystem::Get().open(std::make_shared<boost::filesystem::path>(*p_prj_path / *p_path));
  }
  return p_file->read(buffer, size, offset);
}

bool Path::write(char* buffer, uint64_t size, uint64_t offset) {
  if (!p_file) {
    p_file = FileSystem::Get().open(std::make_shared<boost::filesystem::path>(*p_prj_path / *p_path));
  }
  return p_file->write(buffer, size, offset);
}

uint64_t Path::size() const {
  return p_size;
}

void Path::scanningInfo() {
  p_exist = boost::filesystem::exists(*p_prj_path / *p_path);
  if (p_exist) {
    p_isDir = boost::filesystem::is_directory(*p_prj_path / *p_path);
    p_time  = boost::posix_time::from_time_t(boost::filesystem::last_write_time(*p_prj_path / *p_path));
    if (!p_isDir) {
      p_size = boost::filesystem::file_size(*p_prj_path / *p_path);
    }
  }
}

boost::posix_time::ptime Path::modifyTime() const {
  return p_time;
}

void Path::to_json(nlohmann::json& j) const {
  // date::parse()
  auto str = boost::posix_time::to_iso_string(modifyTime());

  j = nlohmann::json{
      {"path", p_path->generic_string()},
      {"exists", exists()},
      {"isDirectory", isDirectory()},
      {"size", size()},
      {"modifyTime", str}  //
  };
}

void Path::from_json(const nlohmann::json& j) {
  p_prj_path = Project::Get().findPath(j.at("project").get<std::string>());
  auto str   = j.at("path").get<std::string>();
  p_path     = std::make_shared<boost::filesystem::path>(str);
}

DOODLE_NAMESPACE_E