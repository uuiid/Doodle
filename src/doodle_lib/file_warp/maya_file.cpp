#include <Logger/logger.h>
#include <doodle_lib/Exception/exception.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/core/filesystem_extend.h>
#include <doodle_lib/file_warp/maya_file.h>
#include <doodle_lib/lib_warp/boost_locale_warp.h>
#include <doodle_lib/lib_warp/std_warp.h>
#include <doodle_lib/thread_pool/long_term.h>
#include <doodle_lib/thread_pool/thread_pool.h>

#include <boost/locale.hpp>
#include <boost/process.hpp>
#ifdef _WIN32
#include <boost/process/windows.hpp>

#elif defined __linux__
#include <boost/process/posix.hpp>

#endif

namespace doodle {
maya_file::maya_file(FSys::path mayaPath)
    : p_path(std::move(mayaPath)) {
  if (!FSys::exists(p_path) && core_set::getSet().has_maya())
    p_path = core_set::getSet().maya_path();
  else
    throw doodle_error{"无法找到maya启动器"};
}

void maya_file::write_maya_tool_file() {
  const auto tmp_path = core_set::getSet().get_cache_root(
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

FSys::path maya_file::warit_tmp_file(const std::string& in_string) {
  return FSys::write_tmp_file("maya", in_string, ".py");
}

bool maya_file::checkFile() {
  return true;
}

// class ctypr : public std::ctype<char> {
//   mask my_table[table_size];

//  public:
//   ctypr(std::size_t refs = 0)
//       : std::ctype<char>(&my_table[0], false, refs){};
// };

bool maya_file::run_comm(const std::wstring& in_com, const long_term_ptr& in_term) {
  bool k_r{true};
  boost::process::ipstream k_in{};
  boost::process::ipstream k_in2{};
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
  std::string str_r{};
  std::string str_r2{};
  //致命错误。尝试在 C:/Users/ADMINI~1/AppData/Local/Temp/Administrator.20210906.2300.ma 中保存
  const static std::wregex fatal_error_znch{
      LR"(致命错误.尝试在 C:/Users/[a-zA-Z~\d]+/AppData/Local/Temp/[a-zA-Z~\d]+\.\d+\.\d+\.m[ab] 中保存)"};

  // Fatal Error. Attempting to save in C:/Users/Knownexus/AppData/Local/Temp/Knownexus.20160720.1239.ma
  const static std::wregex fatal_error_en_us{
      LR"(Fatal Error\. Attempting to save in C:/Users/[a-zA-Z~\d]+/AppData/Local/Temp/[a-zA-Z~\d]+\.\d+\.\d+\.m[ab])"};

  std::atomic_int16_t k_time_i{0};
  auto k_fun  = std::async([&]() {
    const auto k_time = core_set::getSet().timeout;
    for (; (k_time_i < k_time && k_c.running()); ++k_time_i) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(1s);
    }
    if (k_time_i > k_time) {
      in_term->set_state(long_term::fail);
      auto info_str = "解算文件超时， 请检查文件";
      DOODLE_LOG_WARN(info_str);
      in_term->sig_message_result(info_str, long_term::warning);
      k_r = false;
    }
    k_c.terminate();
    k_in.close();
    k_in2.close();
   });

  auto k_fun2 = std::async([&]() {
    while (k_c.running() && std::getline(k_in, str_r) && !str_r.empty()) {
      k_time_i = 0;
      in_term->sig_message_result(conv::to_utf<char>(str_r, "GBK") + "\n", long_term::warning);
      in_term->sig_progress(rational_int{1, 50});
    }
  });

  while (k_c.running() && std::getline(k_in2, str_r2) && !str_r2.empty()) {
    k_time_i = 0;
    auto str = conv::to_utf<char>(str_r2, "GBK");
    in_term->sig_message_result(str + "\n", long_term::info);
    auto wstr = conv::utf_to_utf<wchar_t>(str);
    if (std::regex_search(wstr.c_str(), fatal_error_znch) || std::regex_search(wstr.c_str(), fatal_error_en_us)) {
      auto info_str = fmt::format("检测到maya结束崩溃,结束进程: 解算命令是 {}", conv::utf_to_utf<char>(in_com));
      DOODLE_LOG_WARN(info_str);
      in_term->sig_message_result(info_str, long_term::warning);
      k_c.terminate();
      in_term->set_state(long_term::fail);
      k_r = false;
    }
    in_term->sig_progress(rational_int{1, 5000});
  }

  return k_r;
}

void maya_file::export_fbx_file(const FSys::path& file_path,
                                const FSys::path& export_path,
                                const long_term_ptr& in_ptr) {
  auto k_arg         = new_object<export_fbx_arg>();
  k_arg->file_path   = file_path;
  k_arg->export_path = export_path;
  k_arg->use_all_ref = false;
  export_fbx_file(k_arg, in_ptr);
}

void maya_file::export_fbx_file(const export_fbx_arg_ptr& in_arg,
                                const long_term_ptr& in_ptr) {
  if (!FSys::exists(in_arg->file_path)) {
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
      R"(# -*- coding: utf-8 -*-\n
import maya_fun_tool
k_f =  maya_fun_tool.open_file()
k_f.config_ = """{}"""
k_f()
quit()
)",
      nlohmann::json{*in_arg}.dump());

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

  std::this_thread::sleep_for(chrono::seconds{3});
  in_ptr->sig_progress(rational_int{1, 40});
  in_ptr->sig_finished();
  if (this->run_comm(conv::utf_to_utf<wchar_t>(run_com), in_ptr)) {
    in_ptr->sig_message_result("导出完成 \n", long_term::warning);
  } else {
    in_ptr->sig_message_result(fmt::format("失败 {}\n", in_arg->file_path), long_term::warning);
  }
}

void maya_file::qcloth_sim_file(const qcloth_arg_ptr& in_arg,
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
      R"(# -*- coding: utf-8 -*-\n
import maya_fun_tool
k_f =  maya_fun_tool.open_file()
k_f.config_ = """{}"""
k_f()
quit())",

      nlohmann::json{*in_arg}.dump());

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

  std::this_thread::sleep_for(chrono::seconds{3});
  in_ptr->sig_progress(rational_int{1, 40});
  in_ptr->sig_finished();
  if (this->run_comm(conv::utf_to_utf<wchar_t>(run_com), in_ptr)) {
    in_ptr->sig_message_result("导出完成 \n", long_term::warning);
  } else {
    in_ptr->sig_message_result(fmt::format("失败 {}\n", in_arg->sim_path), long_term::warning);
  }
}

bool maya_file::is_maya_file(const FSys::path& in_path) {
  auto k_e = in_path.extension();
  return k_e == ".ma" || k_e == ".mb";
}

maya_file_async::maya_file_async()
    : p_maya_file() {}

long_term_ptr maya_file_async::export_fbx_file(const FSys::path& file_path, const FSys::path& export_path) {
  p_maya_file = new_object<maya_file>();
  auto k_term = new_object<long_term>();
  k_term->set_name(file_path.filename().generic_string());
  auto k_f = doodle_lib::Get().get_thread_pool()->enqueue(
      [self = p_maya_file, file_path, export_path, k_term]() {
        self->export_fbx_file(file_path, export_path, k_term);
      });
  k_term->p_list.emplace_back(std::move(k_f));
  return k_term;
}
long_term_ptr maya_file_async::export_fbx_file(maya_file::export_fbx_arg_ptr& in_arg) {
  p_maya_file = new_object<maya_file>();
  auto k_term = new_object<long_term>();
  k_term->set_name(in_arg->file_path.filename().generic_string());
  auto k_f = doodle_lib::Get().get_thread_pool()->enqueue(
      [self = p_maya_file, in_arg, k_term]() {
        self->export_fbx_file(in_arg, k_term);
      });
  k_term->p_list.emplace_back(std::move(k_f));
  return k_term;
}
long_term_ptr maya_file_async::qcloth_sim_file(maya_file::qcloth_arg_ptr& in_arg) {
  p_maya_file = new_object<maya_file>();
  auto k_term = new_object<long_term>();
  k_term->set_name(in_arg->sim_path.filename().generic_string());
  auto k_f = doodle_lib::Get().get_thread_pool()->enqueue(
      [self = p_maya_file, in_arg, k_term]() {
        self->qcloth_sim_file(in_arg, k_term);
      });
  k_term->p_list.emplace_back(std::move(k_f));

  return k_term;
}
}  // namespace doodle
