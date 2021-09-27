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

bool MayaFile::run_comm(const std::wstring& in_com, const long_term_ptr& in_term) {
  boost::process::ipstream k_in{};
  boost::process::ipstream k_in2{};

  // boost::asio::io_context ios{};
  // std::vector<char> v_out(128 << 10);
  // auto out_buff{boost::asio::buffer(v_out)};
  // boost::process::async_pipe pip_out{ios};

  // std::function<void(const boost::system::error_code& ec, std::size_t n)> on_sut;

  // on_sut = [&in_term, &v_out, &pip_out, &out_buff, &on_sut](const boost::system::error_code& ec, std::size_t n) {
  //   in_term->sig_message_result(string{v_out.begin(), v_out.begin() + n}, long_term::warning);
  //   if (!ec) {
  //     boost::asio::async_read(pip_out, out_buff, on_sut);
  //   }
  // };

  // std::vector<char> v_err(128 << 10);
  // auto err_buff{boost::asio::buffer(v_err)};
  // boost::process::async_pipe pip_err{ios};
  // std::function<void(const boost::system::error_code& ec, std::size_t n)> on_err;
  // on_err = [&in_term, &v_err, &pip_err, &err_buff, &on_err](const boost::system::error_code& ec, std::size_t n) {
  //   in_term->sig_message_result(string{v_err.begin(), v_err.begin() + n}, long_term::level::info);
  //   if (!ec) {
  //     boost::asio::async_read(pip_err, err_buff, on_err);
  //   }
  // };

  //   boost::process::child k_c{str.str(), boost::process::windows::hide};
  //   auto com = fmt::format(LR"(cmd.exe /c "{}")",in_com);
  DOODLE_LOG_INFO("命令 {}", boost::locale::conv::utf_to_utf<char>(in_com));
  boost::process::child k_c{
      boost::process::cmd = in_com,
      boost::process::std_out > k_in,
      boost::process::std_err > k_in2
#ifdef _WIN32
      ,
      boost::process::windows::hide
#endif  //_WIN32
  };

  // boost::asio::async_read(pip_err, err_buff, on_err);
  // boost::asio::async_read(pip_out, out_buff, on_sut);
  // ios.run();
  // k_c.wait();
  // return k_c.exit_code() == 0;
  auto fun  = std::async(std::launch::async,
                        [&k_c, &k_in, &in_term]() {
                          auto str_r = std::string{};
                          while (k_c.running()) {
                            if (std::getline(k_in, str_r) && !str_r.empty()) {
                              in_term->sig_message_result(conv::to_utf<char>(str_r, "GBK") + "\n", long_term::warning);
                              in_term->sig_progress(rational_int{1, 50});
                            }
                          }
                        });
  auto fun2 = std::async(std::launch::async,
                         [&k_c, &k_in2, &in_term, &in_com ]() {
                           auto str_r2 = std::string{};
                           //致命错误。尝试在 C:/Users/ADMINI~1/AppData/Local/Temp/Administrator.20210906.2300.ma 中保存
                           const static std::wregex fatal_error_znch{
                               LR"(致命错误.尝试在 C:/Users/[a-zA-Z~\d]+/AppData/Local/Temp/[a-zA-Z~\d]+\.\d+\.\d+\.m[ab] 中保存)"};

                           // Fatal Error. Attempting to save in C:/Users/Knownexus/AppData/Local/Temp/Knownexus.20160720.1239.ma
                           const static std::wregex fatal_error_en_us{
                               LR"(Fatal Error\. Attempting to save in C:/Users/[a-zA-Z~\d]+/AppData/Local/Temp/[a-zA-Z~\d]+\.\d+\.\d+\.m[ab])"};

                           while (k_c.running()) {
                             if (std::getline(k_in2, str_r2) && !str_r2.empty()) {
                               auto str = conv::to_utf<char>(str_r2, "GBK");
                               in_term->sig_message_result(str + "\n", long_term::info);
                               auto wstr = conv::utf_to_utf<wchar_t>(str);
                               if (std::regex_search(wstr.c_str(), fatal_error_znch) || std::regex_search(wstr.c_str(), fatal_error_en_us)) {
                                 auto info_str = fmt::format("检测到maya结束崩溃,结束进程: 解算命令是 {}", conv::utf_to_utf<char>(in_com));
                                 DOODLE_LOG_WARN(info_str);
                                 in_term->sig_message_result(info_str, long_term::warning);
                                 k_c.terminate();
                                 in_term->set_state(long_term::fail);
                               }
                               in_term->sig_progress(rational_int{1, 5000});
                             }
                           }
                         });
  using namespace chrono;
  while (!k_c.wait_for(1s)) {
  }
  return true;
}

void MayaFile::exportFbxFile(const FSys::path& file_path,
                             const FSys::path& export_path,
                             const long_term_ptr& in_ptr) {
  if (!FSys::exists(file_path)) {
    if (in_ptr) {
      in_ptr->sig_finished();
      in_ptr->sig_message_result("不存在文件 \n", long_term::warning);
    }
    return;
  }
  if (in_ptr)
    in_ptr->start();
  write_maya_tool_file();
  if (in_ptr)
    in_ptr->sig_progress(rational_int{1, 40});
  //生成导出文件
  auto str_script = fmt::format(
      "# -*- coding: utf-8 -*-\n"
      "\n"
      "import maya_fun_tool\n"
      "k_f =  maya_fun_tool.open_file(\"{}\")\n"
      "k_f.get_fbx_export()()",
      file_path.generic_string());
  auto run_path = warit_tmp_file(str_script);
  if (in_ptr)
    in_ptr->sig_progress(rational_int{1, 40});

  //生成命令
  auto run_com = fmt::format(
      R"("{}/mayapy.exe" {})",
      this->p_path.generic_string(),
      run_path.generic_string());
  if (in_ptr)
    in_ptr->sig_progress(rational_int{1, 40});

  this->run_comm(conv::utf_to_utf<wchar_t>(run_com), in_ptr);

  if (in_ptr) {
    in_ptr->sig_progress(rational_int{1, 40});
    in_ptr->sig_finished();
    in_ptr->sig_message_result("导出完成 \n", long_term::warning);
  }
}

void MayaFile::qcloth_sim_file(const qcloth_arg_ptr& in_arg,
                               const long_term_ptr& in_ptr) {
  if (!FSys::exists(in_arg->sim_path)) {
    if (in_ptr) {
      in_ptr->sig_finished();
      in_ptr->sig_message_result("不存在文件 \n", long_term::warning);
    }
    return;
  }
  if (in_ptr)
    in_ptr->start();
  // 写入文件
  write_maya_tool_file();
  if (in_ptr)
    in_ptr->sig_progress(rational_int{1, 40});

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
  if (in_ptr)
    in_ptr->sig_progress(rational_int{1, 40});
  //生成命令
  auto run_com = fmt::format(
      R"("{}/mayapy.exe" {})",
      this->p_path.generic_string(),
      run_path.generic_string());
  if (in_ptr)
    in_ptr->sig_progress(rational_int{1, 40});
  run_comm(conv::utf_to_utf<wchar_t>(run_com), in_ptr);
  if (in_ptr) {
    in_ptr->sig_progress(rational_int{1, 40});
    in_ptr->sig_finished();
    in_ptr->sig_message_result(fmt::format("完成导出 :{} \n", in_arg->sim_path.generic_string()), long_term::warning);
  }
}

bool MayaFile::is_maya_file(const FSys::path& in_path) {
  auto k_e = in_path.extension();
  return k_e == ".ma" || k_e == ".mb";
}

maya_file_async::maya_file_async()
    : p_maya_file() {}

long_term_ptr maya_file_async::export_fbx_file(const FSys::path& file_path, const FSys::path& export_path) {
  p_maya_file = std::make_shared<MayaFile>();
  auto k_term = new_object<long_term>();
  k_term->set_name(file_path.filename().generic_string());
  auto k_f = DoodleLib::Get().get_thread_pool()->enqueue(
      [self = p_maya_file, file_path, export_path, k_term]() {
        self->exportFbxFile(file_path, export_path, k_term);
      });
  k_term->p_list.emplace_back(std::move(k_f));
  return k_term;
}
long_term_ptr maya_file_async::qcloth_sim_file(MayaFile::qcloth_arg_ptr& in_arg) {
  p_maya_file = std::make_shared<MayaFile>();
  auto k_term = new_object<long_term>();
  k_term->set_name(in_arg->sim_path.filename().generic_string());
  auto k_f = DoodleLib::Get().get_thread_pool()->enqueue(
      [self = p_maya_file, in_arg, k_term]() {
        self->qcloth_sim_file(in_arg, k_term);
      });
  k_term->p_list.emplace_back(std::move(k_f));

  return k_term;
}
}  // namespace doodle
