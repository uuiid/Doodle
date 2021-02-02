#pragma once

#include <corelib/core_global.h>
#include <map>
#include <regex>
DOODLE_NAMESPACE_S

class Path;
class CORE_API fileDowUpdateOptions {
 public:
  fileDowUpdateOptions();
  ~fileDowUpdateOptions();

  const fileSys::path &locaPath() const noexcept;
  void setlocaPath(const fileSys::path &locaPath) noexcept;

  const fileSys::path &remotePath() const noexcept;
  void setremotePath(const fileSys::path &remotePath) noexcept;

  const bool &Force() const noexcept;
  void setForce(const bool &Force) noexcept;

  const std::vector<std::regex> &Include() const noexcept;
  void setInclude(const std::vector<std::regex> &Include) noexcept;

  const std::vector<std::regex> &Exclude() const noexcept;
  void setExclude(const std::vector<std::regex> &Exclude) noexcept;

 private:
  std::shared_ptr<fileSys::path> p_localFile;
  std::shared_ptr<fileSys::path> p_remoteFile;

  bool p_force;
  std::vector<std::regex> p_includeRegex;
  std::vector<std::regex> p_excludeRegex;
};

DOODLE_NAMESPACE_E