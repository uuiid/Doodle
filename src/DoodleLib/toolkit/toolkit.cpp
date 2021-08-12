#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/FileSys/FileSystem.h>
#include <DoodleLib/FileWarp/ImageSequence.h>
#include <DoodleLib/FileWarp/MayaFile.h>
#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/FileWarp/VideoSequence.h>
#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/Metadata/Shot.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/toolkit/toolkit.h>
#include <ShlObj.h>

#include <boost/algorithm/string.hpp>
#include <regex>
#include <string>
namespace doodle {

void toolkit::installMayaPath() {
  auto mayadoc = CoreSet::getSet().getDoc().parent_path();
  mayadoc /= "maya/modules";

  auto sourePath = CoreSet::program_location().parent_path();
  sourePath /= "plug/maya";

  if (!FSys::exists(mayadoc)) {
    FSys::create_directories(mayadoc);
  } else
    FSys::remove_all(mayadoc);
  FileSystem::localCopy(sourePath, mayadoc, false);
}

void toolkit::installUePath(const FSys::path &path) {
  auto &set      = CoreSet::getSet();
  auto sourePath = set.program_location().parent_path() /
                   "plug/uePlug/" /
                   set.gettUe4Setting().Version() /
                   "/Plugins/Doodle";
  auto targetPath = path / "Plugins/Doodle";

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
  FSys::copy(ue_path, FSys::path{ue_path}.replace_extension(".ini.backup"), FSys::copy_options::update_existing);
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
  return true;
}

}  // namespace doodle
