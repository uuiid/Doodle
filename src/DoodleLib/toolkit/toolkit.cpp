#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/FileSys/FileSystem.h>
#include <DoodleLib/FileWarp/ImageSequence.h>
#include <DoodleLib/FileWarp/MayaFile.h>
#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/FileWarp/VideoSequence.h>
#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/Metadata/Shot.h>
#include <DoodleLib/Metadata/episodes.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/toolkit/toolkit.h>

#include <boost/algorithm/string.hpp>
#include <regex>
#include <string>

#if defined(_WIN32)

#include <ShlObj.h>
#endif

namespace doodle {

void toolkit::installMayaPath() {
  auto mayadoc = CoreSet::getSet().getDoc().parent_path();
  mayadoc /= "maya";
  mayadoc /= "modules";
  mayadoc /= "doodle";

  auto sourePath = CoreSet::program_location().parent_path();
  sourePath /= "plug/maya";

  if (!FSys::exists(mayadoc)) {
    FSys::create_directories(mayadoc);
  } else
    FSys::remove_all(mayadoc);

  FileSystem::localCopy(sourePath, mayadoc, false);
  auto k_tmp_path = mayadoc / "scripts" / "scripts" / "maya_fun_tool.py";
  if (FSys::exists(k_tmp_path))
    return;

  auto k_file_py = cmrc::DoodleLibResource::get_filesystem().open("resource/maya_fun_tool.py");
  {  //写入文件后直接关闭
    FSys::fstream file{k_tmp_path, std::ios::out | std::ios::binary};
    file.write(k_file_py.begin(), boost::numeric_cast<std::int64_t>(k_file_py.size()));
  }

  static std::string k_mod{R"(+ doodle 1.1 .\doodle
MYMODULE_LOCATION:= .
PATH+:= scripts;plug-ins
PYTHONPATH+:= scripts
)"};
  {
    auto k_p = mayadoc.parent_path() / "doodle.mod";
    DOODLE_LOG_INFO("写入 {}", k_p);
    FSys::ofstream k_file{k_p};
    k_file << k_mod;
  }
}

void toolkit::installUePath(const FSys::path &path) {
  auto &set = CoreSet::getSet();

  auto sourePath  = FSys::current_path().parent_path();
  sourePath       = sourePath / "plug" / "uePlug";
  sourePath       = sourePath / set.gettUe4Setting().Version();
  sourePath       = sourePath / "Plugins" / "Doodle";
  auto targetPath = path / "Plugins" / "Doodle";

  if (FSys::exists(targetPath)) {
    FSys::remove_all(targetPath);
  } else {
    FSys::create_directories(targetPath);
  }

  DOODLE_LOG_INFO(fmt::format("install plug : {} --> {}", sourePath, targetPath));
  FileSystem::localCopy(sourePath, targetPath, false);
}

bool toolkit::update() {
  return false;
}

void toolkit::modifyUeCachePath() {
  auto ue_path = CoreSet::getSet().gettUe4Setting().Path() / "Engine/Config/BaseEngine.ini";
  //做备份
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
  //这里我们手动做一些工作
  //获取环境变量
  PWSTR pManager;
  SHGetKnownFolderPath(FOLDERID_LocalAppData, NULL, nullptr, &pManager);
  if (!pManager) throw DoodleError("无法找到保存路径");

  FSys::path path{pManager};
  CoTaskMemFree(pManager);

  path /= "UnrealEngine";
  DOODLE_LOG_INFO(fmt::format("delete Folder : {}", path));
  FSys::remove_all(path);
#endif
  return true;
}

}  // namespace doodle
