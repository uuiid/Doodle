#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/FileWarp/MayaFile.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/core/DoodleLib.h>
#include <DoodleLib/threadPool/ThreadPool.h>
#include <DoodleLib/threadPool/long_term.h>
#include <DoodleLib/core/filesystem_extend.h>
#include <Logger/Logger.h>

#include <boost/locale.hpp>
#include <boost/process.hpp>

namespace doodle {
MayaFile::MayaFile(FSys::path mayaPath)
    : p_path(std::move(mayaPath)),
      p_term(std::make_shared<long_term>()),
      p_term_list(),
      p_futurn_list() {
  if (!FSys::exists(p_path) && CoreSet::getSet().hasMaya())
    p_path = CoreSet::getSet().MayaPath();
  else
    throw DoodleError{"无法找到maya启动器"};
}

void MayaFile::write_maya_tool_file() {
  const static auto tmp_path = CoreSet::getSet().getCacheRoot(
      fmt::format("maya/v{}{}{}",
                  Doodle_VERSION_MAJOR,
                  Doodle_VERSION_MINOR,
                  Doodle_VERSION_PATCH));
  auto k_tmp_path = tmp_path / "maya_fun_tool.py";
  if (FSys::exists(k_tmp_path))
    return;

  auto k_file_py = cmrc::DoodleLibResource::get_filesystem().open("resource/maya_fun_tool.py");
  {  //写入文件后直接关闭
    FSys::fstream file{k_tmp_path, std::ios::out | std::ios::binary};
    file.write(k_file_py.begin(), boost::numeric_cast<std::int64_t>(k_file_py.size()));
  }
}

FSys::path MayaFile::warit_tmp_file(const std::string& in_string) {
  return FSys::write_tmp_file("maya",in_string,".py");
}

bool MayaFile::checkFile() {
  return true;
}

bool MayaFile::run_comm(const std::wstring& in_com) const {
  STARTUPINFO si{};
  PROCESS_INFORMATION pi{};
  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));

  try {
    //使用windowsIPA创建子进程
    CreateProcess(
        nullptr,
        (wchar_t*)in_com.c_str(),
        nullptr,
        nullptr,
        false,
        CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,  // CREATE_NEW_CONSOLE
        nullptr,
        p_path.generic_wstring().c_str(),  // R"(C:\Program Files\Autodesk\Maya2018\bin\)"
        &si,
        &pi);
    p_term->sig_progress(0.1);
    // boost::process::system(command.c_str(), env);
  } catch (const std::runtime_error& err) {
    DOODLE_LOG_WARN(err.what())
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    p_term->sig_finished();
    p_term->sig_message_result("导出失败");
    return false;
  }

  // boost::process::ipstream k_in{};
  // boost::process::child k_c{str.str(), boost::process::windows::hide};
  // boost::process::child k_c{boost::process::cmd = str.str(), boost::process::std_out > k_in, boost::process::windows::hide};

  // auto str_r = std::string{};
  // while (k_c.running() && std::getline(k_in, str_r) && !str_r.empty()) {
  //   // std::cout << str_r << std::endl;
  //   DOODLE_LOG_INFO(str_r);
  // }
  // k_c.wait();

  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return true;
}

[[nodiscard]] long_term_ptr MayaFile::exportFbxFile(const FSys::path& file_path, const FSys::path& export_path) {
  auto k_term = std::make_shared<long_term>();
  p_term_list.push_back(k_term);

  auto k_export_path = file_path.parent_path() / file_path.stem();
  if (!FSys::exists(k_export_path))
    FSys::create_directories(k_export_path);

  if (!FSys::exists(file_path)) {
    k_term->sig_finished();
    k_term->sig_message_result("不存在文件");
    return k_term;
  }
  auto k_fut = DoodleLib::Get().get_thread_pool()->enqueue(
      [this, k_term, file_path, k_export_path]() {
        write_maya_tool_file();
        k_term->sig_progress(0.1);
        //生成导出文件
        auto str_script = fmt::format(
            "# -*- coding: utf-8 -*-\n"
            "\n"
            "import maya.standalone\n"
            "maya.standalone.initialize(name='python')\n"
            "import pymel.core.system\n"
            "import pymel.core\n"
            "pymel.core.system.newFile(force=True)\n"
            "pymel.core.system.loadPlugin(\"fbxmaya\")\n"
            "\n"
            "pymel.core.system.openFile(\"{}\",loadReferenceDepth=\"all\")\n"
            "import maya_fun_tool\n"
            "reload(maya_fun_tool)\n"
            "maya_fun_tool.fbx_export()()",
            file_path.generic_string());
        auto run_path = warit_tmp_file(str_script);
        k_term->sig_progress(0.1);

        //生成命令
        auto run_com = fmt::format(
            LR"("{}/mayapy.exe" {})",
            p_path.generic_wstring(),
            run_path.generic_wstring());
        k_term->sig_progress(0.1);

        run_comm(run_com);

        k_term->sig_progress(0.6);
        FSys::remove(run_path);
        FSys::copy_file(file_path, k_export_path / file_path.filename(), FSys::copy_options::overwrite_existing);
        
        k_term->sig_progress(0.1);
        k_term->sig_finished();
        k_term->sig_message_result("导出完成");
      });
  p_futurn_list.push_back(std::move(k_fut));
  return k_term;
}

long_term_ptr MayaFile::qcloth_sim_file(qcloth_arg_ptr& in_arg) {
  auto k_term = std::make_shared<long_term>();
  p_term_list.push_back(k_term);

  auto k_export_path = in_arg->sim_path.parent_path() / in_arg->sim_path.stem();
  if (!FSys::exists(k_export_path))
    FSys::create_directories(k_export_path);

  if (!FSys::exists(in_arg->sim_path)) {
    k_term->sig_finished();
    k_term->sig_message_result("不存在文件");
    return k_term;
  }

  auto k_fut = DoodleLib::Get().get_thread_pool()->enqueue(
      [in_arg,k_term,this]() {
        // 写入文件
        write_maya_tool_file();
        k_term->sig_progress(0.1);

        auto str_script = fmt::format(
            "import maya.standalone\n"
            "maya.standalone.initialize(name='python')\n"
            "import pymel.core.system\n"
            "import pymel.core\n"
            "pymel.core.system.newFile(force=True)\n"
            "pymel.core.system.loadPlugin(\"AbcExport\")\n"
            "pymel.core.system.loadPlugin(\"AbcImport\")\n"
            "pymel.core.system.loadPlugin(\"qualoth_2019_x64\")"
            "\n\npymel.core.system.openFile(\"{}\",loadReferenceDepth=\"all\")\n"
            "if not pymel.core.mel.eval(\"currentTimeUnitToFPS\") != 25:\n"
            "    quit()"
            "pymel.core.playbackOptions(animationStartTime=\"950\")\n"
            "import maya_fun_tool\n"
            "reload(maya_fun_tool)\n"
            "maya_fun_tool.cloth_export(\"{}\")()",
            in_arg->sim_path.generic_string(),
            in_arg->qcloth_assets_path.generic_string());
        auto run_path = warit_tmp_file(str_script);
        k_term->sig_progress(0.1);
        //生成命令
        auto run_com = fmt::format(
            LR"("{}/mayapy.exe" {})",
            p_path.generic_wstring(),
            run_path.generic_wstring());

        k_term->sig_progress(0.1);
        run_comm(run_com);
        k_term->sig_progress(0.7);
        k_term->sig_finished();
        k_term->sig_message_result(fmt::format("完成导出 :{}", in_arg->sim_path.generic_string()));
      });
  p_futurn_list.push_back(std::move(k_fut));
  return k_term;
}

bool MayaFile::is_maya_file(const FSys::path& in_path) {
  auto k_e = in_path.extension();
  return k_e == ".ma" || k_e == ".mb";
}
long_term_ptr MayaFile::get_term() const {
  return p_term;
}
MayaFile::~MayaFile() {
  for (auto& rus : p_futurn_list) {
    try {
      rus.get();
    } catch (DoodleError& error) {
      DOODLE_LOG_WARN(error.what());
    }
  }
}

}  // namespace doodle
