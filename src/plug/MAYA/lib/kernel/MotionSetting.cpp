#include <lib/kernel/MotionSetting.h>

#include <lib/kernel/Exception.h>

#include <nlohmann/json.hpp>

#include <Windows.h>
#include <ShlObj.h>
namespace doodle::motion::kernel {
std::string MotionSetting::ConfigName = "DoodleMotion.config";

MotionSetting& MotionSetting::Get() {
  static MotionSetting install;
  return install;
}

bool MotionSetting::openMotionProject() {
  if (!FSys::exists(p_motion_path / ConfigName)) return false;

  FSys::fstream file(p_motion_path / ConfigName, std::ios::in | std::ios::binary);
  try {
    auto root           = nlohmann::json::parse(file);
    this->p_motion_name = root["name"].get<std::string>();
  } catch (const nlohmann::json::parse_error& err) {
    std::cout << err.what() << std::endl;
    return false;
  }
  return true;
}

const FSys::path& MotionSetting::MotionLibRoot() const noexcept {
  return p_motion_path;
}

void MotionSetting::setMotionLibRoot(const FSys::path& MotionLibRoot) noexcept {
  if (!FSys::exists(MotionLibRoot))
    FSys::create_directories(MotionLibRoot);

  p_motion_path = MotionLibRoot;
  if (!FSys::exists(p_motion_path / ConfigName)) {
    this->createMotionProject();
  }
}

const std::string& MotionSetting::User() const noexcept {
  return p_user_name;
}

void MotionSetting::setUser(const std::string& User) noexcept {
  p_user_name = User;
}

const std::string& MotionSetting::MotionName() const noexcept {
  return p_motion_name;
}

void MotionSetting::setMotionName(const std::string& MotionName) noexcept {
  p_motion_name = MotionName;
}

void MotionSetting::save() {
  FSys::fstream file{p_setting_path, std::ios::binary | std::ios::out};

  if (!file.is_open()) DoodleError("无法打开文件");
  file << to_json().dump();

  if (!FSys::exists(p_motion_path / ConfigName)) {
    this->createMotionProject();
  }
}

const boost::uuids::uuid MotionSetting::random_generator() {
  return p_uuid();
}

MotionSetting::MotionSetting()
    : p_setting_path(),
      p_motion_path(),
      p_user_name("user"),
      p_motion_name(),
      p_uuid(boost::uuids::random_generator{}) {
  PWSTR pManager;
  SHGetKnownFolderPath(FOLDERID_Documents, NULL, NULL, &pManager);
  if (!pManager) DoodleError("无法找到保存路径");

  p_setting_path = FSys::path{pManager} /
                   "doodle" /
                   "doodleMotion.config.json";
  if (FSys::exists(p_setting_path)) {
    FSys::fstream file{p_setting_path, std::ios::binary | std::ios::in};
    if (!file.is_open()) DoodleError("无法打开文件");
    auto root = nlohmann::json::parse(file);
    from_json(root);
  }
  //写入默认设置
  else {
    FSys::create_directories(p_setting_path.parent_path());
    FSys::fstream file{p_setting_path, std::ios::binary | std::ios::out};
    if (!file.is_open()) DoodleError("无法打开文件");
    file << to_json();
  }
}

void MotionSetting::createMotionProject() {
  auto db_root = p_motion_path / ConfigName;
  if (FSys::exists(db_root)) return;

  nlohmann::json root{};
  root["name"] = p_motion_name;

  FSys::fstream file{db_root, std::ios::out};
  if (!file.is_open()) return;
  file << root.dump();

  FSys::create_directories(p_motion_path / "etc");
  FSys::create_directories(p_motion_path / "fbx");
  FSys::create_directories(p_motion_path / "image");

  InitProject();
}

void MotionSetting::from_json(const nlohmann::json& j) {
  p_motion_path = j.at("MotionLibRoot").get<std::string>();
  p_user_name   = j.at("User").get<std::string>();
}

nlohmann::json MotionSetting::to_json() {
  nlohmann::json root{};
  root["MotionLibRoot"] = p_motion_path.generic_string();
  root["User"]          = p_user_name;
  return root;
}

}  // namespace doodle::motion::kernel