#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/Shot.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/core/DoodleLib.h>
#include <DoodleLib/core/Ue4Setting.h>
#include <DoodleLib/core/filesystem_extend.h>
#include <DoodleLib/threadPool/ThreadPool.h>
#include <DoodleLib/threadPool/long_term.h>

#include <DoodleLib/libWarp/WinReg.hpp>
#include <boost/locale.hpp>
#include <boost/process.hpp>

namespace doodle {

const std::string Ue4Project::Content     = "Content";
const std::string Ue4Project::ContentShot = "Shot";
const std::string Ue4Project::UE4PATH     = "Engine/Binaries/Win64/UE4Editor.exe";
// const std::string Ue4Project::UE4PATH     = "Engine/Binaries/Win64/UE4Editor-Cmd.exe";
const std::string Ue4Project::Character = "Character";
const std::string Ue4Project::Prop      = "Prop";

Ue4Project::Ue4Project(FSys::path project_path)
    : p_ue_path(),
      p_ue_Project_path(std::move(project_path)),
      p_term(std::make_shared<long_term>()),
      p_term_list() {
  auto& ue  = Ue4Setting::Get();
  p_ue_path = ue.Path();
}

void Ue4Project::addUe4ProjectPlugins(const std::vector<std::string>& in_strs) const {
  FSys::ifstream k_ifile{p_ue_Project_path, std::ios::in};

  auto k_ue = nlohmann::json::parse(k_ifile).get<Ue4ProjectFile>();
  for (auto str : in_strs) {
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

void Ue4Project::run_cmd_scipt(const std::string& run_com) const {
  auto k_ue4_cmd = p_ue_path / UE4PATH;
  if (!FSys::exists(k_ue4_cmd))
    throw DoodleError{"找不到ue运行文件"};

  auto k_comm = fmt::format(R"({} {} {})",
                            k_ue4_cmd,          // ue路径
                            p_ue_Project_path,  //项目路径
                            run_com);           // 其后的运行命令

  DOODLE_LOG_INFO(k_comm)
  boost::process::child k_c{k_comm};
  k_c.wait();
}

void Ue4Project::runPythonScript(const std::string& python_str) const {
  auto tmp_name = boost::uuids::to_string(CoreSet::getSet().getUUID()) + ".py";
  auto tmp_file = CoreSet::getSet().getCacheRoot() / tmp_name;

  {  //写入文件
    FSys::ofstream k_ofile{tmp_file};
    k_ofile << python_str;
  }
  runPythonScript(tmp_file);
  FSys::remove(tmp_file);
}

void Ue4Project::runPythonScript(const FSys::path& python_file) const {
  run_cmd_scipt(fmt::format("-ExecutePythonScript={}", python_file));
}

FSys::path Ue4Project::convert_path_to_game(const FSys::path& in_path) const {
  auto k_con = p_ue_Project_path.parent_path() / Content;
  FSys::path k_game_path{"/Game"};
  k_game_path /= in_path.lexically_relative(k_con).parent_path();
  k_game_path /= in_path.stem();
  DOODLE_LOG_INFO("转换路径 {} --> {}", in_path, k_game_path);
  return k_game_path;
}

FSys::path Ue4Project::find_ue4_skeleton(const FSys::path& in_path) {
  static std::regex reg{R"(_(Ch\d+[a-zA-Z])\d?[\._])"};
  std::smatch k_match{};
  auto str      = in_path.generic_string();
  auto k_ch_dir = p_ue_Project_path.parent_path() / Content / Character;

  FSys::path k_r{};

  if (std::regex_search(str, k_match, reg) && FSys::exists(k_ch_dir)) {
    auto k_ch    = k_match[1].str();
    auto k_sk_ch = fmt::format("SK_{}_Skeleton", k_ch);
    auto k_it    = std::find_if(
           FSys::recursive_directory_iterator{k_ch_dir},
           FSys::recursive_directory_iterator{},
           [k_sk_ch](const FSys::directory_entry& in_path) {
          auto k_p = in_path.path().stem().generic_string();
          // std::transform(k_p.begin(), k_p.end(), k_p.begin(), [](unsigned char c) { return std::tolower(c); });
          return in_path.path().stem() == k_sk_ch;
           });
    if (k_it != FSys::recursive_directory_iterator{}) {
      k_r = k_it->path();
      DOODLE_LOG_INFO("寻找到sk {}", k_r);
    }
  }

  if (!k_r.empty()) {
    k_r = convert_path_to_game(k_r);
  }

  return k_r;
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

  auto& set = CoreSet::getSet();
  //创建集数文件夹
  auto k_episodes_path = k_createDir / inShotList[0]->getEpisodesPtr()->str();
  if (!FSys::exists(k_episodes_path))
    FSys::create_directory(k_episodes_path);

  auto k_dep = set.getDepartment();
  //创建特效专用文件夹
  if (set.getDepartmentEnum() == Department::VFX) {
    auto p_episodes_vfx_name = k_episodes_path / k_dep / set.getUser_en();
    if (!FSys::exists(p_episodes_vfx_name))
      FSys::create_directories(p_episodes_vfx_name);
  }

  auto k_tmp_file_path = CoreSet::getSet().getCacheRoot("ue4_lev") / boost::uuids::to_string(CoreSet::getSet().getUUID()).append(".py");

  {  //写入临时文件
    auto tmp_f = cmrc::DoodleLibResource::get_filesystem().open("resource/Ue4CraeteLevel.py");
    FSys::fstream file{k_tmp_file_path, std::ios_base::out | std::ios::binary};
    file.write(tmp_f.begin(), tmp_f.size());
  }
  auto k_prj = inShotList[0]->find_parent_class<Project>();
  {  //这个是后续追加写入
    FSys::fstream file{k_tmp_file_path, std::ios_base::out | std::ios_base::app};

    file << std::endl;

    auto k_game_episodes_path = FSys::path{"/Game"} / ContentShot / inShotList[0]->getEpisodesPtr()->str();
    for (const auto& k_shot : inShotList) {
      auto k_string = fmt::format("{}{:04d}_{}",
                                  k_prj->showStr(),
                                  k_shot->getEpisodesPtr()->getEpisodes(),
                                  k_shot->str());

      auto k_shot_path      = k_episodes_path / k_string;
      auto k_game_shot_path = k_game_episodes_path / k_string;
      if (!FSys::exists(k_shot_path))
        FSys::create_directory(k_shot_path);

      FSys::create_directory(k_shot_path / k_dep);

      //添加关卡序列和定序器
      auto k_shot_su = fmt::format("_{}", k_dep.front());

      auto k_shot_sequence = k_string;
      k_shot_sequence += k_shot_su;

      auto k_shot_sequence_path = k_shot_path / (k_shot_sequence + ".uasset");
      auto k_shot_lev           = k_string;
      k_shot_lev += "_lev" + k_shot_su;
      auto k_shot_lev_path = k_shot_path / (k_shot_lev + ".umap");

      if (!FSys::exists(k_shot_sequence_path) && !FSys::exists(k_shot_lev_path)) {
        file << fmt::format(
                    R"(doodle_lve("""{}""", """{}/""", """{}""", """{}""")())",
                    k_shot_lev,
                    k_game_shot_path.generic_string(),
                    k_shot_sequence,
                    k_game_shot_path.generic_string())
             << std::endl;
      }
    }
    //    file << "time.sleep(3)" << std::endl;
  }

  this->runPythonScript(k_tmp_file_path);
  p_term->sig_finished();
  p_term->sig_message_result("完成添加");
}

long_term_ptr Ue4Project::create_shot_folder_asyn(const std::vector<ShotPtr>& inShotList) {
  auto k_f = DoodleLib::Get().get_thread_pool()->enqueue(
      [this, inShotList]() {
        this->createShotFolder(inShotList);
      });
  p_term->p_list.emplace_back(std::move(k_f));
  return p_term;
}

long_term_ptr Ue4Project::import_files_asyn(const std::vector<FSys::path>& in_paths) {
  this->addUe4ProjectPlugins({"doodle"});

  auto k_term = std::make_shared<long_term>();
  p_term_list.clear();
  for (const auto& i : in_paths) {
    auto k_term = std::make_shared<long_term>();
    p_term_list.push_back(k_term);

    auto k_fun = [this, i, k_term]() {
      nlohmann::json k_root{};
      import_settting k_stting{};
      k_stting.import_file_path     = i;
      k_stting.import_file_save_dir = analysis_path_to_gamepath(i);
      if (i.extension() == ".fbx") {
        k_stting.p_import_type          = import_type::Fbx;
        k_stting.fbx_skeleton_file_name = find_ue4_skeleton(i);
      } else if (i.extension() == ".abc") {
        k_stting.p_import_type = import_type::Abc;
        auto [k_s, k_end]      = FSys::find_path_frame(i);
        k_stting.end_frame     = k_end;
        k_stting.start_frame   = k_s;
      }
      k_root    = k_stting;
      auto path = FSys::write_tmp_file("UE4", k_root.dump(), ".json");
      k_term->sig_progress(rational_int{1, 2});
      run_cmd_scipt(fmt::format("-run=DoodleAssCreate -path={}", path));
      k_term->sig_progress(rational_int{1, 2});
      k_term->sig_finished();
      k_term->sig_message_result(fmt::format("项目 {} 完成导入", p_ue_Project_path));
    };

    k_term->p_list.emplace_back(DoodleLib::Get().get_thread_pool()->enqueue(k_fun));
  }
  k_term->forward_sig(p_term_list);
  return k_term;
}

bool Ue4Project::can_import_ue4(const FSys::path& in_path) {
  auto k_e = in_path.extension();
  return k_e == ".fbx" || k_e == ".abc";
}

bool Ue4Project::is_ue4_file(const FSys::path& in_path) {
  auto k_e = in_path.extension();
  return k_e == ".uproject";
}

FSys::path Ue4Project::analysis_path_to_gamepath(const FSys::path& in_path) {
  std::stringstream k_str{};

  // auto k_p = DoodleLib::Get().p_project_vector;
  if (auto k_ = Episodes::analysis_static(in_path); k_)
    k_str << k_->str();
  if (auto k_ = Shot::analysis_static(in_path); k_) {
    k_str << "_" << k_->str();
  }

  auto k_Dir = FSys::path{"/Game"} / ContentShot;
  if (!k_str.tellp()) {
    k_Dir /= k_str.str();
  }
  return k_Dir;
}

}  // namespace doodle
