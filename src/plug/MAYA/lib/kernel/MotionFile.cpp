#include <lib/kernel/MotionFile.h>

#include <lib/kernel/MotionSetting.h>
#include <nlohmann/json.hpp>

#include <fstream>
namespace doodle::motion::kernel {
void MotionFile::from_json(const nlohmann::json &j) {
  p_Fbx_file  = j["Fbx_file"].get<std::string>();
  p_Gif_file  = j["Gif_file"].get<std::string>();
  p_user_name = j["User_name"].get<std::string>();
  p_info      = j["Info"].get<std::string>();
}

nlohmann::json MotionFile::to_json() {
  nlohmann::json root{};
  root["Fbx_file"]  = p_Fbx_file;
  root["Gif_file"]  = p_Gif_file;
  root["User_name"] = p_user_name;
  root["Info"]      = p_info;
  return root;
}

MotionFile::MotionFile(FSys::path path)
    : p_Fbx_file(std::move(path)),
      p_Gif_file(),
      p_user_name(),
      p_info() {
}

std::vector<MotionFilePtr> MotionFile::FindMotionFiles(const std::string &path) {
  auto &set = MotionSetting::Get();
  auto root = set.MotionLibRoot() / "etc";

  std::vector<MotionFilePtr> lists{};
  for (auto &&item : FSys::directory_iterator(root)) {
    if (item.is_regular_file() && item.path().extension() == ".json") {
      std::fstream file{item.path(), std::ios::in};
      if (!file.is_open()) continue;

      auto json   = nlohmann::json::parse(file);
      auto k_file = std::make_unique<MotionFile>("");
      k_file->from_json(json);
      lists.emplace_back(std::move(k_file));
    }
  }
  return lists;
}

}  // namespace doodle::motion::kernel