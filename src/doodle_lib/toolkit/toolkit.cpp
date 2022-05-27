#include <doodle_core/core/core_set.h>
#include <doodle_lib/toolkit/toolkit.h>

#if defined(_WIN32)

#include <ShlObj.h>
#endif

namespace doodle {

void toolkit::installMayaPath() {
  try {
    auto mayadoc = win::get_pwd();
    mayadoc /= "maya";
    mayadoc /= "modules";
    mayadoc /= "doodle";

    auto sourePath = core_set::program_location().parent_path();
    sourePath /= "maya";

    if (!FSys::exists(mayadoc)) {
      FSys::create_directories(mayadoc);
    } else
      FSys::remove_all(mayadoc);

    DOODLE_LOG_INFO(fmt::format("install plug : {} --> {}", sourePath, mayadoc));
    copy(sourePath, mayadoc, FSys::copy_options::recursive | FSys::copy_options::update_existing);

    static std::string k_mod{R"(+ doodle 1.1 .\doodle
MYMODULE_LOCATION:= .
PATH+:= plug-ins
PYTHONPATH+:= scripts
)"};
    {
      auto k_p = mayadoc.parent_path() / "doodle.mod";
      DOODLE_LOG_INFO("写入 {}", k_p);
      FSys::ofstream k_file{k_p};
      k_file << k_mod;
    }
  } catch (FSys::filesystem_error &err) {
    DOODLE_LOG_ERROR(err.what());
    throw;
  }
}

void toolkit::installUePath(const FSys::path &path) {
  try {
    auto &set      = core_set::getSet();

    auto sourePath = FSys::program_location().parent_path();

    auto l_name{set.ue4_version};
    if (auto l_f = l_name.find('.');
        l_f != std::string::npos) {
      l_name.erase(l_f, 1);
    }
    sourePath /= fmt::format("ue{}_Plug", l_name);
    auto targetPath = path / "Plugins" / "Doodle";

    if (FSys::exists(targetPath)) {
      FSys::remove_all(targetPath);
    } else {
      FSys::create_directories(targetPath);
    }

    DOODLE_LOG_INFO(fmt::format("install plug : {} --> {}", sourePath, targetPath));
    copy(sourePath, targetPath, FSys::copy_options::recursive | FSys::copy_options::update_existing);
  } catch (FSys::filesystem_error &error) {
    DOODLE_LOG_ERROR(error.what());
    throw;
  }
}

void toolkit::modifyUeCachePath() {
  auto ue_path     = core_set::getSet().ue4_path / "Engine/Config/BaseEngine.ini";
  // 做备份
  auto backup_path = FSys::path{ue_path}.replace_extension(".ini.backup");
  FSys::copy(ue_path, FSys::add_time_stamp(backup_path), FSys::copy_options::update_existing);
  FSys::fstream file{ue_path, std::ios::in | std::ios::out | std::ios::binary};
  std::string line{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

  static std::string str{R"("%ENGINEVERSIONAGNOSTICUSERDIR%DerivedDataCache")"};
  auto it = line.find(str);
  while (it != std::string::npos) {
    line.replace(it, str.size(), R"("%GAMEDIR%DerivedDataCache")");
    it = line.find(str);
  }
  file.close();
  file.open(ue_path, std::ios::out | std::ios::trunc | std::ios::binary);
  file << line;
}

bool toolkit::deleteUeCache() {
#if defined(_WIN32)
  // 这里我们手动做一些工作
  // 获取环境变量
  PWSTR pManager;
  SHGetKnownFolderPath(FOLDERID_LocalAppData, NULL, nullptr, &pManager);
  chick_true<doodle_error>(pManager, DOODLE_LOC, "无法找到保存路径");

  FSys::path path{pManager};
  CoTaskMemFree(pManager);

  path /= "UnrealEngine";
  DOODLE_LOG_INFO(fmt::format("delete Folder : {}", path));
  FSys::remove_all(path);
#endif
  return true;
}

}  // namespace doodle
