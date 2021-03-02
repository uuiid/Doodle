#include <lib/kernel/MotionFile.h>

#include <lib/kernel/MotionSetting.h>
#include <nlohmann/json.hpp>

#include <fstream>
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
  root["Fbx_file"]  = p_Fbx_file;
  root["Gif_file"]  = p_Gif_file;
  root["title"]     = p_title;
  root["User_name"] = p_user_name;
  root["Info"]      = p_info;
  return root;
}

MotionFile::MotionFile(FSys::path path)
    : p_Fbx_file(std::move(path)),
      p_Gif_file(),
      p_title(),
      p_user_name(),
      p_info() {
}

const FSys::path& MotionFile::FbxFile() const noexcept {
  return p_Fbx_file;
}

void MotionFile::setFbxFile(const FSys::path& FbxFile) noexcept {
  p_Fbx_file = FbxFile;
}

const FSys::path& MotionFile::GifFile() const noexcept {
  return p_Gif_file;
}

void MotionFile::setGifFile(const FSys::path& GifFile) noexcept {
  p_Gif_file = GifFile;
}

const std::string& MotionFile::UserName() const noexcept {
  return p_user_name;
}

void MotionFile::setUserName(const std::string& UserName) noexcept {
  p_user_name = UserName;
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

std::vector<MotionFilePtr> MotionFile::FindMotionFiles(const std::string& path) {
  auto& set = MotionSetting::Get();
  auto root = set.MotionLibRoot() / "etc";

  if (!path.empty())
    root /= path;

  std::vector<MotionFilePtr> lists{};
  nlohmann::json k_json{};
  for (auto&& item : FSys::directory_iterator(root)) {
    if (item.is_regular_file() && item.path().extension() == ".json") {
      std::fstream file{item.path(), std::ios::in};
      if (!file.is_open()) continue;

      k_json      = nlohmann::json::parse(file);
      auto k_file = std::make_unique<MotionFile>("");
      k_file->from_json(k_json);
      lists.emplace_back(std::move(k_file));
    }
  }
  return lists;
}

}  // namespace doodle::motion::kernel