#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/core/Ue4Setting.h>
#include <DoodleLib/core/coreset.h>
#include <DoodleLib/Metadata/Shot.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/libWarp/WinReg.hpp>

#include <boost/format.hpp>
#include <boost/locale.hpp>
#include <boost/process.hpp>

namespace doodle {

const std::string Ue4Project::Content     = "Content";
const std::string Ue4Project::ContentShot = "Shot";
const std::string Ue4Project::UE4PATH     = "Engine/Binaries/Win64/UE4Editor-Cmd.exe";
const std::string Ue4Project::Character   = "Character";

Ue4Project::Ue4Project(FSys::path project_path)
    : p_ue_path(),
      p_ue_Project_path(std::move(project_path)),
      p_project(MetadataSet::Get().Project_()) {
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
  auto tmp_file = coreSet::getSet().getCacheRoot() / tmp_name;

  {  //写入文件
    FSys::ofstream k_ofile{tmp_file};
    k_ofile << python_str;
  }
  runPythonScript(tmp_file);
  FSys::remove(tmp_file);
}

void Ue4Project::runPythonScript(const FSys::path& python_file) const {
  auto k_ue4_cmd = p_ue_path / UE4PATH;
  if (!FSys::exists(k_ue4_cmd))
    throw DoodleError{"找不到ue运行文件"};
  //  boost::format command{R"("%s" "%s" -run=%s -script="%s")"};
  boost::format command{R"("%s" "%s" -ExecutePythonScript="%s")"};

  command % k_ue4_cmd.generic_string()      //ue路径
      % p_ue_Project_path.generic_string()  //项目路径
                                            //      % "pythonscript"                      //运行ue命令名称
      % python_file.generic_string()        //python脚本路径
      ;
  DOODLE_LOG_INFO(command.str())
  boost::process::child k_c{command.str()};
  k_c.wait();
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

  auto k_tmp_file_path = coreSet::getSet().getCacheRoot("ue4_lev") / boost::uuids::to_string(coreSet::getSet().getUUID()).append(".py");

  {  //写入临时文件
    auto tmp_f = cmrc::DoodleLibResource::get_filesystem().open("resource/Ue4CraeteLevel.py");
    FSys::fstream file{k_tmp_file_path, std::ios_base::out | std::ios::binary};
    file.write(tmp_f.begin(), tmp_f.size());
  }

  {  //这个是后续追加写入
    FSys::fstream file{k_tmp_file_path, std::ios_base::out | std::ios_base::app};
    boost::format python_format{R"(doodle_lve("%s", "%s/", "%s", "%s/")())"};

    file << std::endl;

    auto k_game_episodes_path = FSys::path{"/Game"} / ContentShot / inShotList[0]->Episodes_()->str();
    for (const auto& k_shot : inShotList) {
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
      k_shot_sequence += k_shot_suffix.str();

      auto k_shot_sequence_path = k_shot_path / (k_shot_sequence + ".uasset");
      auto k_shot_lev           = k_shot_str.str();
      k_shot_lev += "_lev" + k_shot_suffix.str();
      auto k_shot_lev_path = k_shot_path / (k_shot_lev + ".umap");

      if (!FSys::exists(k_shot_sequence_path) && !FSys::exists(k_shot_lev_path)) {
        python_format % k_shot_lev % k_game_shot_path.generic_string() % k_shot_sequence % k_game_shot_path.generic_string();

        file << python_format.str() << std::endl;
        python_format.clear();
      }
    }
    //    file << "time.sleep(3)" << std::endl;
  }

  this->runPythonScript(k_tmp_file_path);
}

}  // namespace doodle
