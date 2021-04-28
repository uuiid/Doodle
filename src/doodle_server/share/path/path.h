#pragma once
#include <doodle_server/DoodleServer_global.h>
#include <nlohmann/json.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <optional>
DOODLE_NAMESPACE_S
class IoFile;
class Path {
 public:
  Path();
  Path(std::string& str);
  virtual ~Path();

  FSys::path* path() const;
  void setPath(const std::string& path_str);
  bool exists() const;
  bool isDirectory() const;

  bool createFolder() const;

  bool read(char* buffer, uint64_t size, uint64_t offset);
  bool write(char* buffer, uint64_t size, uint64_t offset);
  bool rename(const Path& newName);
  bool copy(const Path& target);

  std::optional<std::vector<std::shared_ptr<Path>>> list() const;

  uint64_t size() const;

  void scanningInfo();

  boost::posix_time::ptime modifyTime() const;

 private:
  friend void to_json(nlohmann::json& j, const Path& p);
  friend void from_json(const nlohmann::json& j, Path& p);
  friend class nlohmann::basic_json<>;

  void to_json(nlohmann::json& j) const;
  void from_json(const nlohmann::json& j);

  std::shared_ptr<FSys::path> p_path;
  std::shared_ptr<FSys::path> p_prj_path;

  bool p_exist;
  bool p_isDir;
  uint64_t p_size;
  boost::posix_time::ptime p_time;
  std::shared_ptr<IoFile> p_file;
};

inline static void to_json(nlohmann::json& j, const Path& p) {
  p.to_json(j);
};
inline static void from_json(const nlohmann::json& j, Path& p) {
  p.from_json(j);
};

DOODLE_NAMESPACE_E