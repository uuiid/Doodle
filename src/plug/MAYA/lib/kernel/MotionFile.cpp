#include <lib/kernel/MotionFile.h>

#include <lib/kernel/Maya/FbxExport.h>
#include <lib/kernel/MotionSetting.h>
#include <lib/kernel/Exception.h>
#include <lib/kernel/BoostUuidWarp.h>
#include <lib/kernel/Maya/Screenshot.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>

namespace doodle::motion::kernel {
void MotionFile::from_json(const nlohmann::json& j) {
  p_Fbx_file  = j["Fbx_file"].get<std::string>();
  p_Gif_file  = j["Gif_file"].get<std::string>();
  p_title     = j["title"].get<std::string>();
  p_user_name = j["User_name"].get<std::string>();
  p_info      = j["Info"].get<std::string>();
}

nlohmann::json MotionFile::to_json() {
  nlohmann::json root{};
  root["Fbx_file"]  = p_Fbx_file.generic_u8string();
  root["Gif_file"]  = p_Gif_file.generic_u8string();
  root["title"]     = p_title;
  root["User_name"] = p_user_name;
  root["Info"]      = p_info;
  return root;
}

MotionFile::MotionFile()
    : p_file(),
      p_Fbx_file(),
      p_Gif_file(),
      p_title(),
      p_user_name(MotionSetting::Get().User()),
      p_info() {
}

const FSys::path& MotionFile::FbxFile() const noexcept {
  return p_Fbx_file;
}

const FSys::path& MotionFile::GifFile() const noexcept {
  return p_Gif_file;
}

const std::string& MotionFile::UserName() const noexcept {
  return p_user_name;
}

const std::string& MotionFile::Info() const noexcept {
  return p_info;
}

void MotionFile::setInfo(const std::string& Info) noexcept {
  p_info = Info;
}

const std::string& MotionFile::Title() const noexcept {
  return p_title;
}

void MotionFile::setTitle(const std::string& Title) noexcept {
  p_title = Title;
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
    auto k_uuid     = boost::uuids::random_generator{}();
    auto k_uuid_str = boost::uuids::to_string(k_uuid);

    auto& set = MotionSetting::Get();

    //产生路径和创建路径
    auto k_path_fbx  = set.MotionLibRoot() / "fbx";
    auto k_path_josn = set.MotionLibRoot() / "etc";
    auto k_path_gif  = set.MotionLibRoot() / "image";

    for (auto&& k_pathItem : relativePath) {
      if (k_pathItem == *(relativePath.begin()))
        continue;
      k_path_fbx /= k_pathItem;
      k_path_josn /= k_pathItem;
      k_path_gif /= k_pathItem;
    }

    k_path_fbx /= (k_uuid_str + ".fbx");
    k_path_josn /= (k_uuid_str + ".json");
    k_path_gif /= (k_uuid_str + ".mp4");

    if (!FSys::exists(k_path_fbx.parent_path()))
      FSys::create_directories(k_path_fbx.parent_path());
    if (!FSys::exists(k_path_josn.parent_path()))
      FSys::create_directories(k_path_josn.parent_path());
    if (!FSys::exists(k_path_gif.parent_path()))
      FSys::create_directories(k_path_gif.parent_path());

    //导出fbx
    // auto k_status = FbxExport::FbxExportMEL(k_path_fbx);
    // if (k_status != MStatus::MStatusCode::kSuccess) throw FbxFileError("无法导出文件");
    auto k_screen = Screenshot{k_path_gif};
    if (!k_path_gif.empty())
      k_screen.save(MTime{0, MTime::uiUnit()}, MTime{30, MTime::uiUnit()});

    this->p_file     = std::move(k_path_josn);
    this->p_Fbx_file = std::move(k_path_fbx);
    this->p_Gif_file = std::move(k_path_gif);
  }

  //写出配置文件
  auto k_file = std::fstream{this->p_file, std::ios::out | std::ios::binary};
  if (!k_file.is_open()) std::runtime_error("无法打开文件");

  k_file << this->to_json();
}

}  // namespace doodle::motion::kernel