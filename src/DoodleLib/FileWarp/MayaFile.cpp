#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/FileWarp/MayaFile.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/core/DoodleLib.h>
#include <DoodleLib/core/filesystem_extend.h>
#include <DoodleLib/libWarp/boost_locale_warp.h>
#include <DoodleLib/libWarp/std_warp.h>
#include <DoodleLib/threadPool/ThreadPool.h>
#include <DoodleLib/threadPool/long_term.h>
#include <Logger/Logger.h>

#include <boost/locale.hpp>
#include <boost/process.hpp>
#ifdef _WIN32
#include <boost/process/windows.hpp>

#elif defined __linux__
#include <boost/process/posix.hpp>

#endif

namespace doodle {
MayaFile::MayaFile(FSys::path mayaPath)
    : p_path(std::move(mayaPath)) {
  if (!FSys::exists(p_path) && CoreSet::getSet().hasMaya())
    p_path = CoreSet::getSet().MayaPath();
  else
    throw DoodleError{"无法找到maya启动器"};
}

void MayaFile::write_maya_tool_file() {
  const auto tmp_path = CoreSet::getSet().getCacheRoot(
      fmt::format("maya\\v{}{}{}",
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
  return FSys::write_tmp_file("maya", in_string, ".py");
}

bool MayaFile::checkFile() {
  return true;
}

bool MayaFile::run_comm(const std::wstring& in_com, const long_term_ptr& in_term) const {
  boost::process::ipstream k_in{};
  boost::process::ipstream k_in2{};
  //   boost::process::child k_c{str.str(), boost::process::windows::hide};
  //   auto com = fmt::format(LR"(cmd.exe /c "{}")",in_com);
  DOODLE_LOG_INFO("命令 {}", boost::locale::conv::utf_to_utf<char>(in_com));
  boost::process::child k_c{
      boost::process::cmd = in_com,
      boost::process::std_out > k_in,
      boost::process::std_err > k_in2,
      boost::process::std_in.close()
#ifdef _WIN32
          ,
      boost::process::windows::hide
#endif  //_WIN32
  };

  auto fun    = std::async(std::launch::async,
                           [&k_c, &k_in, &in_term]() {
                          auto str_r = std::string{};
                          while (k_c.running() && std::getline(k_in, str_r) && !str_r.empty()) {
                            in_term->sig_message_result(boost::locale::conv::to_utf<char>(str_r, "GB18030") + "\n", long_term::warning);
                            in_term->sig_progress(rational_int{1, 1000});
                          }
                           });
  auto str_r2 = std::string{};
  //致命错误。尝试在 C:/Users/ADMINI~1/AppData/Local/Temp/Administrator.20210906.2300.ma 中保存
  const static std::wregex fatal_error_znch{
      LR"(致命错误.尝试在 C:/Users/[a-zA-Z~\d]+/AppData/Local/Temp/[a-zA-Z~\d]+\.\d+\.\d+\.ma 中保存)"};

  // Fatal Error. Attempting to save in C:/Users/Knownexus/AppData/Local/Temp/Knownexus.20160720.1239.ma
  const static std::wregex fatal_error_en_us{
      LR"(Fatal Error\. Attempting to save in C:/Users/[a-zA-Z~\d]+/AppData/Local/Temp/[a-zA-Z~\d]+\.\d+\.\d+\.ma)"};

  while (k_c.running() && std::getline(k_in2, str_r2) && !str_r2.empty()) {
    auto str = boost::locale::conv::to_utf<char>(str_r2, "GB18030");
    in_term->sig_message_result(str+ "\n", long_term::info);
    auto wstr = boost::locale::conv::utf_to_utf<wchar_t>(str);
    if (std::regex_search(wstr.c_str(), fatal_error_znch) || std::regex_search(wstr.c_str(), fatal_error_en_us)) {
      DOODLE_LOG_WARN("检测到maya结束崩溃,结束进程: 解算命令是 {}", boost::locale::conv::utf_to_utf<char>(in_com));
      boost::process::system(fmt::format("taskkill /F /T /PID {}", k_c.id()));
    }
    in_term->sig_progress(rational_int{1, 100});
  }
  k_c.wait();
  fun.get();

  return true;
}

[[nodiscard]] long_term_ptr MayaFile::exportFbxFile(const FSys::path& file_path, const FSys::path& export_path) {
  auto k_term = make_shared_<long_term>();

  if (!FSys::exists(file_path)) {
    k_term->sig_finished();
    k_term->sig_message_result("不存在文件 \n",long_term::warning);
    return k_term;
  }
  auto k_fut = DoodleLib::Get().get_thread_pool()->enqueue(
      [self = shared_from_this(), k_term, file_path]() {
        write_maya_tool_file();
        k_term->sig_progress(rational_int{1, 10});
        //生成导出文件
        auto str_script = fmt::format(
            "# -*- coding: utf-8 -*-\n"
            "\n"
            "import maya_fun_tool\n"
            "k_f =  maya_fun_tool.open_file(\"{}\")\n"
            "k_f.get_fbx_export()()",
            file_path.generic_string());
        auto run_path = warit_tmp_file(str_script);
        k_term->sig_progress(rational_int{1, 10});

        //生成命令
        auto run_com = fmt::format(
            R"("{}/mayapy.exe" {})",
            self->p_path.generic_string(),
            run_path.generic_string());
        k_term->sig_progress(rational_int{1, 10});

        self->run_comm(conv::utf_to_utf<wchar_t>(run_com), k_term);

        k_term->sig_progress(rational_int{1, 60});
        FSys::remove(run_path);

        k_term->sig_progress(rational_int{1, 10});
        k_term->sig_finished();
        k_term->sig_message_result("导出完成 \n",long_term::warning);
      });
  k_term->p_list.push_back(std::move(k_fut));
  return k_term;
}

long_term_ptr MayaFile::qcloth_sim_file(qcloth_arg_ptr& in_arg) {
  auto k_term = std::make_shared<long_term>();

  auto k_export_path = in_arg->sim_path.parent_path() / in_arg->sim_path.stem();
  if (!FSys::exists(k_export_path))
    FSys::create_directories(k_export_path);

  if (!FSys::exists(in_arg->sim_path)) {
    k_term->sig_finished();
    k_term->sig_message_result("不存在文件 \n",long_term::warning);
    return k_term;
  }

  auto k_fut = DoodleLib::Get().get_thread_pool()->enqueue(
      [in_arg, k_term, self = shared_from_this()]() {
        // 写入文件
        write_maya_tool_file();
        k_term->sig_progress(rational_int{1, 10});

        auto str_script = fmt::format(
            "# -*- coding: utf-8 -*-\n"
            "import maya_fun_tool\n"
            "k_f =  maya_fun_tool.open_file(\"{}\")\n"
            "sim = k_f.get_cloth_sim(\"{}\")\n",
            in_arg->sim_path.generic_string(),
            in_arg->qcloth_assets_path.generic_string());
        if (in_arg->only_sim)
          str_script.append("sim.sim_and_export()\n");
        else
          str_script.append("sim()\n");

        auto run_path = warit_tmp_file(str_script);
        k_term->sig_progress(rational_int{1, 10});
        //生成命令
        auto run_com = fmt::format(
            R"("{}/mayapy.exe" {})",
            self->p_path.generic_string(),
            run_path.generic_string());

        k_term->sig_progress(rational_int{1, 10});
        self->run_comm(conv::utf_to_utf<wchar_t>(run_com), k_term);
        k_term->sig_progress(rational_int{1, 70});
        k_term->sig_finished();
        k_term->sig_message_result(fmt::format("完成导出 :{} \n", in_arg->sim_path.generic_string()),long_term::warning);
      });
  k_term->p_list.push_back(std::move(k_fut));
  return k_term;
}

bool MayaFile::is_maya_file(const FSys::path& in_path) {
  auto k_e = in_path.extension();
  return k_e == ".ma" || k_e == ".mb";
}

}  // namespace doodle
