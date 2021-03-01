#pragma once

#include <MotionGlobal.h>
#include <nlohmann/json_fwd.hpp>

namespace doodle::motion::kernel {
class MotionFile;
using MotionFilePtr = std::shared_ptr<MotionFile>;
class MotionFile {
 private:
  FSys::path p_Fbx_file;
  FSys::path p_Gif_file;
  std::string p_user_name;
  std::string p_info;

  void from_json(const nlohmann::json& j);
  nlohmann::json to_json();

 public:
  MotionFile(FSys::path path);

  static std::vector<MotionFilePtr> FindMotionFiles(const std::string& path);
};

}  // namespace doodle::motion::kernel