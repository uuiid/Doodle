#include "path.h"
#include <doodle_server/source/Project.h>

#include <doodle_server/source/FileSystem.h>
#include <loggerlib/Logger.h>
#include <boost/filesystem.hpp>
#include <chrono>
DOODLE_NAMESPACE_S
Path::Path(std::string& str)
    : p_path(nullptr),
      p_prj_path(nullptr),
      p_exist(false),
      p_isDir(false),
      p_size(0),
      p_time(boost::posix_time::min_date_time) {
  p_path     = std::make_shared<FSys::path>(str);
  p_prj_path = std::make_shared<FSys::path>("");
  scanningInfo();
}

Path::Path()
    : p_path(std::make_shared<FSys::path>("")),
      p_prj_path(std::make_shared<FSys::path>("")),
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
  return FSys::create_directories(*p_path);
}

bool Path::read(char* buffer, uint64_t size, uint64_t offset) {
  DOODLE_LOG_INFO(p_path->generic_string()
                  << " size: " << size
                  << " offset: " << offset);
  if (!p_file) {
    p_file = FileSystem::Get().open(std::make_shared<FSys::path>(*p_path));
  }
  return p_file->read(buffer, size, offset);
}

bool Path::write(char* buffer, uint64_t size, uint64_t offset) {
  DOODLE_LOG_INFO(p_path->generic_string()
                  << " size: " << size
                  << " offset: " << offset);
  if (!p_file) {
    p_file = FileSystem::Get().open(std::make_shared<FSys::path>(*p_path));
  }
  return p_file->write(buffer, size, offset);
}

bool Path::rename(const Path& newName) {
  DOODLE_LOG_INFO(p_path->generic_path()
                  << " rename --> "
                  << newName.p_path->generic_string());
  auto status = FileSystem::Get()
                    .rename(p_path.get(), newName.p_path.get());
  if (status) {
    *p_prj_path = *newName.p_prj_path;
    *p_path     = *newName.p_path;
    scanningInfo();
  }
  return status;
}

bool Path::copy(const Path& target) {
  DOODLE_LOG_INFO(p_path->generic_path()
                  << " copy file --> "
                  << target.p_path->generic_string());
  auto status = FileSystem::Get().copy(p_path.get(), target.p_path.get());
  return status;
}

std::optional<std::vector<std::shared_ptr<Path>>> Path::list() const {
  DOODLE_LOG_INFO(p_path->generic_path());
  if (p_isDir) {
    std::vector<std::shared_ptr<Path>> paths{};
    for (auto&& it : FSys::directory_iterator(*p_path)) {
      auto k_path        = std::make_shared<Path>();
      *(k_path->p_path)  = it.path();
      k_path->p_prj_path = p_prj_path;
      k_path->scanningInfo();
      paths.push_back(k_path);
    }
    return paths;
  } else {
    return {};
  }
}

uint64_t Path::size() const {
  return p_size;
}

void Path::scanningInfo() {
  DOODLE_LOG_INFO(p_path->generic_string())
  p_exist = FSys::exists(*p_path);
  if (p_exist) {
    p_isDir = FSys::is_directory(*p_path);

    try {
      p_time = boost::posix_time::from_time_t(FSys::last_write_time(*p_path));
    } catch (const FSys::filesystem_error& e) {
      DOODLE_LOG_ERROR(e.what());
    } catch (const std::exception& e) {
      DOODLE_LOG_ERROR(e.what());
    } catch (...) {
      DOODLE_LOG_ERROR("未知异常: ");
    }

    if (!p_isDir) {
      p_size = FSys::file_size(*p_path);
    }
  }
}

boost::posix_time::ptime Path::modifyTime() const {
  return p_time;
}

void Path::to_json(nlohmann::json& j) const {
  // date::parse()
  auto str = boost::posix_time::to_iso_string(modifyTime());

  auto path_str = p_path->generic_string();
  auto find_str = p_prj_path->generic_string();
  auto find_pos = path_str.find_first_of(find_str);

  path_str.replace(find_pos, find_str.size(), "");

  j = nlohmann::json{
      {"path", path_str},
      {"exists", exists()},
      {"isDirectory", isDirectory()},
      {"size", size()},
      {"modifyTime", str}  //
  };
}

void Path::from_json(const nlohmann::json& j) {
  p_prj_path = Project::Get().findPath(j.at("project").get<std::string>());
  auto str   = j.at("path").get<std::string>();
  p_path     = std::make_shared<FSys::path>(*p_prj_path / str);
}

DOODLE_NAMESPACE_E