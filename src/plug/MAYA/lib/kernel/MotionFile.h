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

  void save();

 public:
  DOODLE_DISABLE_COPY(MotionFile);

  explicit MotionFile();
  ~MotionFile();

  const FSys::path& FbxFile() const noexcept;
  const bool hasIconFile() const noexcept;
  const FSys::path& IconFile() const noexcept;
  const bool hasvideoFile() const noexcept;
  const FSys::path& VideoFile() const noexcept;
  const std::string& UserName() const noexcept;

  const std::string& Info() const noexcept;
  void setInfo(const std::string& Info) noexcept;

  const std::string& Title() const noexcept;
  void setTitle(const std::string& Title) noexcept;

  static std::vector<MotionFilePtr> getAll(const FSys::path& path);
  //创建fbx这个是必须调用的
  void createFbxFile(const FSys::path& relativePath);
  //在创建fbx后要更改时调用
  void createIconFile();
  void createVideoFile();
};

}  // namespace doodle::motion::kernel

#include <QtCore/QVariant>
Q_DECLARE_OPAQUE_POINTER(doodle::motion::kernel::MotionFile)
Q_DECLARE_METATYPE(doodle::motion::kernel::MotionFile*)