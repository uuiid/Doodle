#pragma once
#include <DoodleServer_global.h>
#include <nlohmann/json.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

DOODLE_NAMESPACE_S

class Path {
 public:
  Path(std::string& str);
  Path();
  virtual ~Path();

  boost::filesystem::path* path() const;
  void setPath(const std::string& path_str);
  bool exists() const;
  bool isDirectory() const;

  bool createFolder() const;

  uint64_t size() const;

  void scanningInfo();

  boost::posix_time::ptime modifyTime() const;

  friend void to_json(nlohmann::json& j, const Path& p);
  friend void from_json(const nlohmann::json& j, Path& p);

 private:
  void to_json(nlohmann::json& j) const;
  void from_json(const nlohmann::json& j);
  std::shared_ptr<boost::filesystem::path> p_path;

  bool p_exist;
  bool p_isDir;
  uint64_t p_size;
  boost::posix_time::ptime p_time;
};

inline static void to_json(nlohmann::json& j, const Path& p) {
  p.to_json(j);
};
inline static void from_json(const nlohmann::json& j, Path& p) {
  p.from_json(j);
};

DOODLE_NAMESPACE_E