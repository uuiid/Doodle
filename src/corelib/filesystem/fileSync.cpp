#include <corelib/filesystem/fileSync.h>
#include <corelib/filesystem/Path.h>
#include <corelib/filesystem/FileSystem.h>
#include <boost/filesystem.hpp>
#include <queue>
DOODLE_NAMESPACE_S
fileDowUpdateOptions::fileDowUpdateOptions()
    : p_localFile(std::make_shared<fileSys::path>()),
      p_remoteFile(std::make_shared<fileSys::path>()),
      p_force(false),
      p_includeRegex(),
      p_excludeRegex() {
}

fileDowUpdateOptions::~fileDowUpdateOptions() {
}

const fileSys::path& fileDowUpdateOptions::locaPath() const noexcept {
  return *p_localFile;
}

void fileDowUpdateOptions::setlocaPath(const fileSys::path& locaPath) noexcept {
  *p_localFile = locaPath;
}

const fileSys::path& fileDowUpdateOptions::remotePath() const noexcept {
  return *p_remoteFile;
}

void fileDowUpdateOptions::setremotePath(const fileSys::path& remotePath) noexcept {
  *p_remoteFile = remotePath;
}

const bool& fileDowUpdateOptions::Force() const noexcept {
  return p_force;
}

void fileDowUpdateOptions::setForce(const bool& Force) noexcept {
  p_force = Force;
}

const std::vector<std::regex>& fileDowUpdateOptions::Include() const noexcept {
  return p_includeRegex;
}

void fileDowUpdateOptions::setInclude(const std::vector<std::regex>& Include) noexcept {
  p_includeRegex = Include;
}

const std::vector<std::regex>& fileDowUpdateOptions::Exclude() const noexcept {
  return p_excludeRegex;
}

void fileDowUpdateOptions::setExclude(const std::vector<std::regex>& Exclude) noexcept {
  p_excludeRegex = Exclude;
}

// fileSync::fileSync() {
// }

// fileSync::~fileSync() {
// }

// void fileSync::setPair(const std::pair<std::string, std::string>& pair) {
//   p_pair.first  = boost::filesystem::path{pair.first}.lexically_normal().generic_string();
//   p_pair.second = boost::filesystem::path{pair.second}.lexically_normal().generic_string();
// }

// void fileSync::setInclude(const std::vector<std::string>& includeRegex) {
//   p_includeRegex.clear();
//   for (auto&& item : includeRegex) {
//     p_includeRegex.push_back(std::regex{item});
//   }
// }

// void fileSync::setExclude(const std::vector<std::string>& excludeRegex) {
//   p_excludeRegex.clear();
//   for (auto&& item : excludeRegex) {
//     p_excludeRegex.push_back(std::regex{item});
//   }
// }

// void fileSync::seanDir() {
//   std::regex k_loca_regex{p_pair.first};
//   for (auto&& item : boost::filesystem::directory_iterator{p_pair.first}) {
//     auto key           = std::regex_replace(item.path().generic_string(), k_loca_regex, "");
//     p_paths[key].first = std::make_shared<Path>(item.path().generic_string());
//   }

//   std::regex k_remove_regex{p_pair.second};
//   std::queue<Path> pathQueue;
//   auto& k_fsys    = DfileSyntem::get();
//   auto serverPath = k_fsys.getInfo(&fileSys::path{p_pair.second});
//   pathQueue.push(Path{*serverPath});

// }

// void fileSync::syncFir() {
// }

DOODLE_NAMESPACE_E
