#include <lib/kernel/MotionFile.h>

#include <lib/kernel/Maya/FbxExport.h>
#include <lib/kernel/MotionSetting.h>
#include <lib/kernel/Exception.h>
#include <lib/kernel/BoostUuidWarp.h>
#include <lib/kernel/Maya/Screenshot.h>
#include <lib/kernel/Maya/MayaVideo.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>

namespace doodle::motion::kernel {
void MotionFile::from_json(const nlohmann::json& j) {
  p_Fbx_file   = j["Fbx_file"].get<std::string>();
  p_Gif_file   = j["Gif_file"].get<std::string>();
  p_video_file = j["Video_file"].get<std::string>();
  p_title      = j["title"].get<std::string>();
  p_user_name  = j["User_name"].get<std::string>();
  p_info       = j["Info"].get<std::string>();
}

nlohmann::json MotionFile::to_json() {
  nlohmann::json root{};
  root["Fbx_file"]   = p_Fbx_file.generic_u8string();
  root["Gif_file"]   = p_Gif_file.generic_u8string();
  root["Video_file"] = p_video_file.generic_u8string();
  root["title"]      = p_title;
  root["User_name"]  = p_user_name;
  root["Info"]       = p_info;
  return root;
}

void MotionFile::save() {
  auto k_file = std::fstream{this->p_file, std::ios::out | std::ios::binary};
  if (!k_file.is_open()) std::runtime_error("无法打开文件");

  k_file << this->to_json();
}

MotionFile::MotionFile()
    : p_file(),
      p_Fbx_file(),
      p_Gif_file(),
      p_video_file(),
      p_title(),
      p_user_name(MotionSetting::Get().User()),
      p_info(),
      dataChanged(),
      notDeleteFile() {
}

MotionFile::~MotionFile() {
}

const FSys::path& MotionFile::FbxFile() const noexcept {
  return p_Fbx_file;
}

const bool MotionFile::hasIconFile() const noexcept {
  return FSys::exists(p_Gif_file);
}

const FSys::path& MotionFile::IconFile() const noexcept {
  return p_Gif_file;
}

const bool MotionFile::hasvideoFile() const noexcept {
  return FSys::exists(p_Gif_file);
}

const FSys::path& MotionFile::VideoFile() const noexcept {
  return p_video_file;
}

const std::string& MotionFile::UserName() const noexcept {
  return p_user_name;
}

const std::string& MotionFile::Info() const noexcept {
  return p_info;
}

void MotionFile::setInfo(const std::string& Info) noexcept {
  p_info = Info;
  this->save();
}

const std::string& MotionFile::Title() const noexcept {
  return p_title;
}

void MotionFile::setTitle(const std::string& Title) noexcept {
  p_title = Title;
  this->save();
}

std::vector<MotionFilePtr> MotionFile::getAll(const FSys::path& path) {
  std::vector<MotionFilePtr> lists{};

  if (!FSys::is_directory(path)) return lists;

  nlohmann::json k_json{};
  for (auto& item : FSys::directory_iterator(path)) {
    if (item.is_regular_file() && item.path().extension() == ".json") {
      std::fstream file{item.path(), std::ios::in};
      if (!file.is_open()) continue;

      k_json      = nlohmann::json::parse(file);
      auto k_file = std::make_unique<MotionFile>();
      k_file->from_json(k_json);
      k_file->p_file = item.path();
      lists.emplace_back(std::move(k_file));
    }
  }

  return lists;
}

void MotionFile::createFbxFile(const FSys::path& relativePath) {
  {
    //这个域中是创建临时变量导出文件
    auto k_uuid_str = boost::uuids::to_string(MotionSetting::Get().random_generator());

    auto& set = MotionSetting::Get();

    //产生路径和创建路径
    auto k_path_fbx   = set.MotionLibRoot() / "fbx";
    auto k_path_josn  = set.MotionLibRoot() / "etc";
    auto k_path_gif   = set.MotionLibRoot() / "image";
    auto k_path_video = set.MotionLibRoot() / "video";

    for (auto&& k_pathItem : relativePath) {
      if (k_pathItem == *(relativePath.begin()))
        continue;
      k_path_fbx /= k_pathItem;
      k_path_josn /= k_pathItem;
      k_path_gif /= k_pathItem;
      k_path_video /= k_pathItem;
    }

    k_path_fbx /= (k_uuid_str + ".fbx");
    k_path_josn /= (k_uuid_str + ".json");
    k_path_gif /= (k_uuid_str + ".png");
    k_path_video /= (k_uuid_str + ".mp4");

    for (auto& k_tmp : std::vector<FSys::path>{k_path_fbx,
                                               k_path_josn,
                                               k_path_gif,
                                               k_path_video}) {
      if (!FSys::exists(k_tmp.parent_path()))
        FSys::create_directories(k_tmp.parent_path());
    }

    //导出fbx
    auto k_status = FbxExport::FbxExportMEL(k_path_fbx);
    if (k_status != MStatus::MStatusCode::kSuccess) throw FbxFileError("无法导出文件");
    if (!FSys::exists(k_path_fbx)) throw FbxFileError("未找到导出文件");

    this->p_file       = std::move(k_path_josn);
    this->p_Fbx_file   = std::move(k_path_fbx);
    this->p_Gif_file   = std::move(k_path_gif);
    this->p_video_file = std::move(k_path_video);

    this->createIconFile();
    this->createVideoFile();
  }

  this->dataChanged(this, 0);
  this->dataChanged(this, 1);
  this->dataChanged(this, 2);

  //写出配置文件
  this->save();
}

void MotionFile::createIconFile() {
  if (!FSys::exists(this->p_Fbx_file)) throw FbxFileError("未找到导出文件");
  // p_Gif_file.replace_filename();
  auto k_file = p_Gif_file;
  // if (FSys::exists(p_Gif_file))
  p_Gif_file.replace_filename(boost::uuids::to_string(MotionSetting::Get().random_generator()) + ".png");

  auto k_screen = Screenshot{p_Gif_file};
  k_screen.save();
  this->save();

  //在这里可能抛出无法删除的异常放在最后
  if (FSys::exists(k_file))
    try {
      FSys::remove(k_file);
    } catch (const FSys::filesystem_error& err) {
      std::cout << err.what() << std::endl;
      this->notDeleteFile(k_file);
    }
  this->dataChanged(this, 1);
}

void MotionFile::createVideoFile() {
  if (!FSys::exists(this->p_Fbx_file)) throw FbxFileError("未找到导出文件");

  //暂存临时变量
  auto k_file = p_video_file;
  // if (FSys::exists(p_video_file))
  p_video_file.replace_filename(boost::uuids::to_string(MotionSetting::Get().random_generator()) + ".mp4");
  //保存视频文件
  auto k_video = MayaVideo{p_video_file};
  k_video.save();
  this->save();

  if (FSys::exists(k_file))
    try {
      FSys::remove(k_file);
    } catch (const FSys::filesystem_error& err) {
      std::cout << err.what() << std::endl;
      this->notDeleteFile(k_file);
    }
  this->dataChanged(this, 2);
}

}  // namespace doodle::motion::kernel