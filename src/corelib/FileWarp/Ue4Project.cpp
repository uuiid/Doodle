#include <corelib/FileWarp/Ue4Project.h>
#include <corelib/Exception/Exception.h>
#include <corelib/core/Ue4Setting.h>
#include <corelib/core/coreset.h>
#include <corelib/Metadata/Shot.h>
#include <corelib/Metadata/Episodes.h>

#include <loggerlib/Logger.h>

#include <corelib/libWarp/WinReg.hpp>
#include <boost/format.hpp>
#include <boost/locale.hpp>
#include <boost/process.hpp>

namespace doodle {

const std::string Ue4Project::Content     = "Content";
const std::string Ue4Project::ContentShot = "Shot";
const std::string Ue4Project::UE4PATH     = "Engine/Binaries/Win64/UE4Editor-Cmd.exe";

Ue4Project::Ue4Project(FSys::path project_path)
    : p_ue_path(),
      p_ue_Project_path(std::move(project_path)),
      p_project(coreSet::getSet().Project_()) {
  auto& ue  = Ue4Setting::Get();
  p_ue_path = ue.Path();
}

void Ue4Project::addUe4ProjectPlugins(const std::vector<std::string>& strs) const {
  FSys::ifstream k_ifile{p_ue_Project_path, std::ios::in};

  auto k_ue = nlohmann::json::parse(k_ifile).get<Ue4ProjectFile>();
  for (auto str : strs) {
    auto it = std::find_if(
        k_ue.Plugins.begin(), k_ue.Plugins.end(),
        [&str](const Ue4ProjectFilePulgins& plug) { return plug.Name == str; });
    if (it == k_ue.Plugins.end())
      k_ue.Plugins.emplace_back(Ue4ProjectFilePulgins{str, true});
  }

  nlohmann::json root = k_ue;

  k_ifile.close();
  FSys::ofstream k_ofile{p_ue_Project_path};
  k_ofile << root;
}

void Ue4Project::runPythonScript(const std::string& python_str) const {
  auto tmp_name = boost::uuids::to_string(coreSet::getSet().getUUID()) + ".py";
  auto tmp_file = FSys::temp_directory_path() / "doodle" / tmp_name;

  {  //写入文件
    FSys::ofstream k_ofile{tmp_file};
    k_ofile << python_str;
  }

  auto k_ue4_cmd = p_ue_path / UE4PATH;
  if (!FSys::exists(k_ue4_cmd))
    throw DoodleError{"找不到ue运行文件"};
  boost::format command{R"("%s" "%s" -run=%s -script="%s")"};
  command % k_ue4_cmd.generic_string()      //ue路径
      % p_ue_Project_path.generic_string()  //项目路径
      % "pythonscript"                      //运行ue命令名称
      % tmp_file.generic_string()           //python脚本路径
      ;
  DOODLE_LOG_INFO(command.str());
  boost::process::child k_c{command.str()};
  k_c.wait();
  FSys::remove(tmp_file);
}

void Ue4Project::runPythonScript(const FSys::path& python_file) const {
}

void Ue4Project::createShotFolder(const std::vector<ShotPtr>& inShotList) {
  if (inShotList.empty())
    return;

  //添加python插件
  this->addUe4ProjectPlugins({"PythonScriptPlugin", "SequencerScripting"});

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
  std::string python_str{R"(import unreal
)"};
  boost::format python_format_LevelSequence{R"(ass = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    asset_name='%s',
    package_path='/Game/%s', 
    asset_class=unreal.LevelSequence, 
    factory=unreal.LevelSequenceFactoryNew())
ass.make_range(1001,1200)
ass.set_playback_start(1001)
ass.set_playback_end(1200)

ass.set_work_range_start(40)
ass.set_work_range_end(30)

ass.set_view_range_start(30)
ass.set_view_range_end(30)
unreal.EditorAssetLibrary.save_directory('/Game/')
)"};
  boost::format python_format_World{R"(ass_lev = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    asset_name='%1%', 
    package_path='/Game/%2%', 
    asset_class=unreal.World, 
    factory=unreal.WorldFactory())
unreal.EditorLoadingAndSavingUtils.save_map(ass_lev, "/Game/%2%/%1%")
)"};

  auto k_game_episodes_path = FSys::path{ContentShot} / inShotList[0]->Episodes_()->str();
  for (auto k_shot : inShotList) {
    boost::format k_shot_str{"%s%04d_%s"};
    k_shot_str %
        this->p_project->ShortStr() %
        k_shot->Episodes_()->Episodes_() %
        k_shot->str();

    auto k_shot_path      = k_episodes_path / k_shot_str.str();
    auto k_game_shot_path = k_game_episodes_path / k_shot_str.str();
    if (!FSys::exists(k_shot_path))
      FSys::create_directory(k_shot_path);

    FSys::create_directory(k_shot_path / k_dep);

    //添加关卡序列和定序器

    boost::format k_shot_suffix{"_%s"};
    k_shot_suffix % k_dep.front();

    auto k_shot_sequence = k_shot_str.str();
    k_shot_sequence      = k_shot_sequence + k_shot_suffix.str();

    FSys::ofstream file{};
    auto k_shot_sequence_path = k_shot_path / (k_shot_sequence + ".uasset");
    if (!FSys::exists(k_shot_sequence_path)) {
      python_format_LevelSequence % k_shot_sequence % k_game_shot_path.generic_string();
      python_str.append(python_format_LevelSequence.str());
      python_format_LevelSequence.clear();
    }

    auto k_shot_lev      = k_shot_str.str();
    k_shot_lev           = k_shot_lev + "_lev" + k_shot_suffix.str();
    auto k_shot_lev_path = k_shot_path / (k_shot_lev + ".umap");
    if (!FSys::exists(k_shot_lev_path)) {
      python_format_World % k_shot_lev % k_game_shot_path.generic_string();
      python_str.append(python_format_World.str());
      python_format_World.clear();
    }
  }
  this->runPythonScript(python_str);
}

}  // namespace doodle