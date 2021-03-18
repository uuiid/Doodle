#pragma once

#include <MotionGlobal.h>
#include <nlohmann/json_fwd.hpp>

#include <boost/signals2.hpp>

namespace doodle::motion::kernel {

class MotionFile : public std::enable_shared_from_this<MotionFile> {
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
  // 0 .dataChangded
  const FSys::path& FbxFile() const noexcept;
  const bool hasIconFile() const noexcept;
  // 1 .dataChangded
  const FSys::path& IconFile() const noexcept;
  const bool hasvideoFile() const noexcept;
  // 2 .dataChangded
  const FSys::path& VideoFile() const noexcept;
  const std::string& UserName() const noexcept;
  // 3 .dataChangded
  const std::string& Info() const noexcept;
  void setInfo(const std::string& Info) noexcept;

  const std::string& Title() const noexcept;
  void setTitle(const std::string& Title) noexcept;

  static std::vector<MotionFilePtr> getAll(const FSys::path& path);
  //创建fbx这个是必须调用的
  void createFbxFile(const FSys::path& relativePath);
  //导入fbx调用
  void importFbxFile();
  //在创建fbx后要更改时调用
  void createIconFile();
  void createVideoFile();

  enum class InsideData {
    FbxFile,
    Title,
    User,
    Inco,
    Video,
    Info,
  };

  boost::signals2::signal<void(const MotionFile*, InsideData)> dataBeginChanged;
  boost::signals2::signal<void(const MotionFile*, const int&)> dataChanged;
  boost::signals2::signal<void(const FSys::path&)> notDeleteFile;
};
using MotionFile_QVar = MotionFile*;
}  // namespace doodle::motion::kernel

#include <QtCore/QVariant>
Q_DECLARE_OPAQUE_POINTER(doodle::motion::kernel::MotionFile)
Q_DECLARE_METATYPE(doodle::motion::kernel::MotionFile_QVar)