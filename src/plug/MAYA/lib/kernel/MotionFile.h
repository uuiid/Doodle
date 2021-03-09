#pragma once

#include <MotionGlobal.h>
#include <nlohmann/json_fwd.hpp>

namespace doodle::motion::kernel {
class MotionFile;
using MotionFilePtr = std::shared_ptr<MotionFile>;
class MotionFile {
 private:
  FSys::path p_file;
  FSys::path p_Fbx_file;
  FSys::path p_Gif_file;
  FSys::path p_video_file;

  std::string p_title;
  std::string p_user_name;
  std::string p_info;

  void from_json(const nlohmann::json& j);
  nlohmann::json to_json();

 public:
  DOODLE_DISABLE_COPY(MotionFile);

  explicit MotionFile();

  const FSys::path& FbxFile() const noexcept;
  const FSys::path& GifFile() const noexcept;
  const std::string& UserName() const noexcept;

  const std::string& Info() const noexcept;
  void setInfo(const std::string& Info) noexcept;

  const std::string& Title() const noexcept;
  void setTitle(const std::string& Title) noexcept;

  static std::vector<MotionFilePtr> getAll(const FSys::path& path);
  void createFbxFile(const FSys::path& relativePath);
};

}  // namespace doodle::motion::kernel