#pragma once

#include <corelib/core_global.h>
#include <nlohmann/json.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

DOODLE_NAMESPACE_S

namespace FileSystem {
class CORE_API Path {
 public:
  Path();
  Path(std::string str);
  Path(std::shared_ptr<fileSys::path> path);

  const std::shared_ptr<fileSys::path> &path() const noexcept;
  void setPath(const std::shared_ptr<fileSys::path> &Path_) noexcept;

  void scanningInfo();

  const bool &exists() const;
  const bool &isDirectory() const;
  const uint64_t &size() const;
  const std::chrono::time_point<std::chrono::system_clock> &modifyTime() const;

  void rename(const Path &path);
  void copy(const Path &target);
  void create();
  std::vector<std::shared_ptr<Path>> list();
  std::unique_ptr<std::fstream> open(std::ios_base::openmode modle);

 private:
  std::shared_ptr<fileSys::path> p_path;
  bool p_exist;
  bool p_isDir;
  uint64_t p_size;
  std::chrono::time_point<std::chrono::system_clock> p_time;
};

};  // namespace FileSystem
DOODLE_NAMESPACE_E