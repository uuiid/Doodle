#pragma once

#include <fileSystem/fileSystem_global.h>
#include <nlohmann/json.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

DSYSTEM_S
class Path {
 public:
  Path(const std::string &pathstr);
  Path();
  virtual ~Path();

  const bool &Exist() const noexcept;
  const bool &isDirectory() const noexcept;
  const uint64_t &size() const noexcept;
  const boost::posix_time::ptime &modifyTime() const noexcept;

  const std::shared_ptr<fileSys::path> &path() const noexcept;
  void setPath(const std::shared_ptr<fileSys::path> &Path_) noexcept;

 private:
  std::shared_ptr<fileSys::path> p_path;
  bool p_exist;
  bool p_isDir;
  uint64_t p_size;
  boost::posix_time::ptime p_time;

  friend void to_json(nlohmann::json &j, const Path &p);
  friend void from_json(const nlohmann::json &j, Path &p);
  friend class nlohmann::basic_json<>;
};
enum class fileOptions {
  getInfo      = 0,
  createFolder = 1,
  update       = 2,
  down         = 3,
  rename       = 4,
  list         = 5,
};
void to_json(nlohmann::json &j, const Path &p);
void from_json(const nlohmann::json &j, Path &p);
DSYSTEM_E