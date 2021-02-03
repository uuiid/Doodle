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

  using regex_ptr = std::shared_ptr<std::regex>;

  const fileSys::path &locaPath() const noexcept;
  void setlocaPath(const fileSys::path &locaPath) noexcept;

  const fileSys::path &remotePath() const noexcept;
  void setremotePath(const fileSys::path &remotePath) noexcept;

  const bool &Force() const noexcept;
  void setForce(const bool &Force) noexcept;

  const std::vector<regex_ptr> &Include() const noexcept;
  void setInclude(const std::vector<regex_ptr> &Include) noexcept;

  const std::vector<regex_ptr> &Exclude() const noexcept;
  void setExclude(const std::vector<regex_ptr> &Exclude) noexcept;

 private:
  std::shared_ptr<fileSys::path> p_localFile;
  std::shared_ptr<fileSys::path> p_remoteFile;

  bool p_force;
  std::vector<regex_ptr> p_includeRegex;
  std::vector<regex_ptr> p_excludeRegex;
};

DOODLE_NAMESPACE_E