#include <corelib/FileWarp/Ue4Project.h>
#include <corelib/Exception/Exception.h>
#include <corelib/libWarp/WinReg.hpp>
#include <corelib/core/Ue4Setting.h>
#include <corelib/core/coreset.h>
#include <corelib/Metadata/Shot.h>
#include <corelib/Metadata/Episodes.h>

#include <loggerlib/Logger.h>

#include <boost/format.hpp>
#include <boost/locale.hpp>
namespace doodle {

const std::string Ue4Project::Content     = "Content";
const std::string Ue4Project::ContentShot = "Shot";

Ue4Project::Ue4Project(FSys::path project_path)
    : p_ue_path(),
      p_ue_Project_path(std::move(project_path)),
      p_project(coreSet::getSet().Project_()) {
  auto& ue = Ue4Setting::Get();
  if (ue.hasPath()) {
    p_ue_path = ue.Path();
  } else {
    auto key_str = boost::wformat{LR"(SOFTWARE\EpicGames\Unreal Engine\%s)"};
    auto wv      = boost::locale::conv::utf_to_utf<wchar_t>(Ue4Setting::Get().Version());
    key_str % wv;

    auto key  = winreg::RegKey{HKEY_LOCAL_MACHINE, key_str.str()};
    p_ue_path = FSys::path{key.GetStringValue(L"InstalledDirectory")};
  }
}

void Ue4Project::createShotFolder(const std::vector<ShotPtr>& inShotList) {
  if (inShotList.empty())
    return;

  auto k_createDir = p_ue_Project_path.parent_path() / Content / ContentShot;
  if (!FSys::exists(k_createDir)) {
    FSys::create_directories(k_createDir);
  }

  auto& set = coreSet::getSet();
  //创建集数文件夹
  auto k_episodes_path = k_createDir / inShotList[0]->Episodes_()->str();
  if (!FSys::exists(k_episodes_path))
    FSys::create_directory(k_episodes_path);

  auto k_dep = set.getDepartment();
  //创建特效专用文件夹
  if (set.getDepartmentEnum() == Department::VFX) {
    auto p_episodes_vfx_name = k_episodes_path / k_dep / set.getUser_en();
    if (!FSys::exists(p_episodes_vfx_name))
      FSys::create_directories(p_episodes_vfx_name);
  }

  //创建镜头文件夹
  for (auto k_shot : inShotList) {
    boost::format k_shot_str{"%s%04d_%s"};
    k_shot_str %
        this->p_project->ShortStr() %
        k_shot->Episodes_()->Episodes_() %
        k_shot->str();

    auto k_shot_path = k_episodes_path / k_shot_str.str();
    if (!FSys::exists(k_shot_path))
      FSys::create_directory(k_shot_path);

    FSys::create_directory(k_shot_path / k_dep);

    //添加关卡序列和定序器

    boost::format k_shot_suffix{"_%s"};
    k_shot_suffix % k_dep.front();

    auto k_shot_sequence = k_shot_str.str();
    k_shot_sequence      = k_shot_sequence + k_shot_suffix.str() + ".uasset";

    FSys::ofstream file{};
    auto k_shot_sequence_path = k_shot_path / k_shot_sequence;
    if (!FSys::exists(k_shot_sequence_path)) {
      file.open(k_shot_sequence_path, std::ios::badbit | std::ios::out);
      auto k_file = cmrc::CoreResource::get_filesystem().open("resource/NewLevelSequence.uasset");
      file.write(k_file.begin(), k_file.size());
      file.close();
    }

    auto k_shot_lev      = k_shot_str.str();
    k_shot_lev           = k_shot_lev + "_lev" + k_shot_suffix.str() + ".umap";
    auto k_shot_lev_path = k_shot_path / k_shot_lev;
    if (!FSys::exists(k_shot_lev_path)) {
      file.open(k_shot_lev_path, std::ios::badbit | std::ios::out);
      auto k_file = cmrc::CoreResource::get_filesystem().open("resource/NewWorld.umap");
      file.write(k_file.begin(), k_file.size());
      file.close();
    }
  }
}

}  // namespace doodle