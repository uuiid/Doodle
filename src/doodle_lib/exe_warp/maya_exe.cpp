//
// Created by TD on 2021/12/25.
//

#include "maya_exe.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/thread_pool/process_message.h>

#include <doodle_lib/core/filesystem_extend.h>

#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <stack>
// #include <type_traits>

#include <boost/process.hpp>
#include <boost/process/extend.hpp>
#ifdef _WIN32
#include <boost/process/windows.hpp>
#elif defined __linux__
#include <boost/process/posix.hpp>
#endif
namespace doodle {
namespace {
// 致命错误。尝试在 C:/Users/ADMINI~1/AppData/Local/Temp/Administrator.20210906.2300.ma 中保存
constexpr const auto fatal_error_znch{
    LR"(致命错误.尝试在 C:/Users/[a-zA-Z~\d]+/AppData/Local/Temp/[a-zA-Z~\d]+\.\d+\.\d+\.m[ab] 中保存)"};

// Fatal Error. Attempting to save in C:/Users/Knownexus/AppData/Local/Temp/Knownexus.20160720.1239.ma
constexpr const auto fatal_error_en_us{
    LR"(Fatal Error\. Attempting to save in C:/Users/[a-zA-Z~\d]+/AppData/Local/Temp/[a-zA-Z~\d]+\.\d+\.\d+\.m[ab])"};

}  // namespace
namespace maya_exe_ns {

FSys::path find_maya_work(const FSys::path &in_file_path) {
  if (FSys::exists(in_file_path.parent_path() / "workspace.mel")) {
    return in_file_path.parent_path();
  }
  if (FSys::exists(in_file_path.parent_path().parent_path() / "workspace.mel")) {
    return in_file_path.parent_path().parent_path();
  }
  return in_file_path.parent_path();
}

class run_maya : public std::enable_shared_from_this<run_maya> {
 public:
  entt::handle mag_attr{};
  FSys::path file_path_attr{};

  std::string run_script_attr{};

  boost::process::async_pipe out_attr{g_io_context()};
  boost::process::async_pipe err_attr{g_io_context()};
  boost::process::child child_attr{};

  std::string out_str_attr{};
  std::string err_str_attr{};

  std::shared_ptr<std::function<void(boost::system::error_code)>> call_attr{};

  boost::asio::high_resolution_timer timer_attr{g_io_context()};

  run_maya() = default;
  virtual ~run_maya() { cancel(); }

  void run() {
    auto l_maya     = core_set::get_set().maya_path() / "mayapy.exe";
    auto l_tmp_file = FSys::write_tmp_file("maya", run_script_attr, ".py");
    auto &&l_msg    = mag_attr.get<process_message>();
    l_msg.set_state(l_msg.run);
    l_msg.set_name(file_path_attr.filename().generic_string());
    l_msg.message(fmt::format("开始写入配置文件 {} \n", l_tmp_file), l_msg.warning);
    l_msg.aborted_function = [this]() {
      auto &&l_msg = mag_attr.get<process_message>();
      l_msg.set_state(l_msg.fail);
      l_msg.message("进程被主动结束");
      cancel();
    };

    timer_attr.expires_from_now(chrono::seconds{core_set::get_set().timeout});
    timer_attr.async_wait([this](boost::system::error_code in_code) {
      if (!in_code) {
        auto &&l_msg = mag_attr.get<process_message>();
        l_msg.set_state(l_msg.fail);
        l_msg.message("进程超时，结束任务");
        child_attr.terminate();
      } else {
        DOODLE_LOG_ERROR(in_code);
      }
    });

    child_attr = boost::process::child{
        g_io_context(),
        boost::process::exe  = l_maya,
        boost::process::args = l_tmp_file.generic_wstring(),
        boost::process::std_out > out_attr,
        boost::process::std_err > err_attr,
        boost::process::on_exit = [this](int in_exit, const std::error_code &in_error_code) {
          timer_attr.cancel();
          boost::ignore_unused(in_exit);
          (*call_attr)(in_error_code);
        }};

    read_out();
    read_err();
  }

  void read_out() {
    boost::asio::async_read_until(
        out_attr, boost::asio::dynamic_buffer(out_str_attr), '\n',
        [this, l_self = shared_from_this()](boost::system::error_code in_code, std::size_t in_n) {
          auto &&l_msg = mag_attr.get<process_message>();
          timer_attr.expires_from_now(chrono::seconds{core_set::get_set().timeout});
          if (!in_code) {
            /// @brief 此处在主线程调用
            auto l_str = conv::to_utf<char>(out_str_attr, "GBK");
            l_msg.progress_step({1, 300});
            l_msg.message(l_str, l_msg.warning);
            out_str_attr.clear();
            read_out();
          } else {
            out_attr.close();
            l_msg.message(in_code.message());
            DOODLE_LOG_ERROR(in_code);
          }
        }
    );
  }
  void read_err() {
    boost::asio::async_read_until(
        err_attr, boost::asio::dynamic_buffer(err_str_attr), '\n',
        [this, l_self = shared_from_this()](boost::system::error_code in_code, std::size_t in_n) {
          auto &&l_msg = mag_attr.get<process_message>();
          timer_attr.expires_from_now(chrono::seconds{core_set::get_set().timeout});
          if (!in_code) {
            /// @brief 此处在主线程调用
            // 致命错误。尝试在 C:/Users/ADMINI~1/AppData/Local/Temp/Administrator.20210906.2300.ma 中保存
            const static std::wregex l_fatal_error_znch{fatal_error_znch};
            // Fatal Error. Attempting to save in C:/Users/Knownexus/AppData/Local/Temp/Knownexus.20160720.1239.ma
            const static std::wregex l_fatal_error_en_us{fatal_error_en_us};
            auto l_w_str = conv::to_utf<wchar_t>(err_str_attr, "GBK");
            if (std::regex_search(l_w_str, l_fatal_error_znch) || std::regex_search(l_w_str, l_fatal_error_en_us)) {
              DOODLE_LOG_WARN("检测到maya结束崩溃,结束进程: 解算文件是 {}\n", file_path_attr);
              auto l_mstr = fmt::format("检测到maya结束崩溃,结束进程: 解算文件是 {}\n", file_path_attr);
              l_msg.message(l_mstr, l_msg.warning);
              return;
            } else {
              auto l_str = conv::to_utf<char>(err_str_attr, "GBK");
              l_msg.progress_step({1, 20000});
              l_msg.message(l_str);
              err_str_attr.clear();
              read_err();
            }
          } else {
            err_attr.close();
            l_msg.message(in_code.message());
            DOODLE_LOG_ERROR(in_code);
          }
        }
    );
  }

  void cancel() {
    timer_attr.cancel();
    child_attr.terminate();
  }
};

std::string qcloth_arg::to_str() const {
  nlohmann::json l_json{};
  l_json = *this;
  return fmt::format(
      R"(# -*- coding: utf-8 -*-
import maya_fun_tool
k_f =  maya_fun_tool.open_file()
k_f.config_ = """{}"""
k_f()
)",
      l_json.dump()
  );
}

std::string export_fbx_arg::to_str() const {
  nlohmann::json l_json{};
  l_json = *this;
  return fmt::format(
      R"(# -*- coding: utf-8 -*-
import maya_fun_tool
k_f =  maya_fun_tool.open_file()
k_f.config_ = """{}"""
k_f()
)",
      l_json.dump()
  );
}

std::string replace_file_arg::to_str() const {
  nlohmann::json l_json{};
  l_json = *this;
  return fmt::format(
      R"(# -*- coding: utf-8 -*-
import maya_fun_tool
k_f =  maya_fun_tool.open_file()
k_f.config_ = """{}"""
k_f()
)",
      l_json.dump()
  );
}

std::string clear_file_arg::to_str() const {
  auto l_save_file_path =
      file_path.parent_path() / "fbx" / file_path.filename().replace_extension(save_file_extension_attr);
  return fmt::format(
      R"(# -*- coding: utf-8 -*-
import maya.mel
from maya import cmds
cmds.file(force=True, new=True)

l_file_path = "{}"
save_file_path = "{}"
project_path = "{}"
work_path = "{}"
doodle_plug = "doodle_maya_" + str(cmds.about(api=True))[0:4]
cmds.loadPlugin(doodle_plug)
cmds.workspace(work_path, openWorkspace=1)

cmds.doodle_load_project(project=project_path)
cmds.file(l_file_path, open=True)

cmds.doodle_clear_scene(err_4=True)
cmds.doodle_comm_file_save(filepath=save_file_path)
)",
      file_path, l_save_file_path, project_, find_maya_work(file_path).generic_string()
  );
}

}  // namespace maya_exe_ns

class maya_exe::impl {
 public:
  std::stack<std::shared_ptr<maya_exe_ns::run_maya>> run_process_arg_attr;
  std::vector<std::shared_ptr<maya_exe_ns::run_maya>> run_attr{};

  std::atomic_char16_t run_size_attr{};
};

maya_exe::maya_exe() : p_i(std::make_unique<impl>()) {}

void maya_exe::add_maya_fun_tool() {
  auto l_file_py = cmrc::DoodleLibResource::get_filesystem().open("resource/maya_fun_tool.py");
  FSys::write_tmp_file(
      "maya", std::string{l_file_py.begin(), boost::numeric_cast<std::size_t>(l_file_py.size())}, ".py",
      "maya_fun_tool", std::ios::out | std::ios::binary
  );
}
void maya_exe::notify_run() {
  add_maya_fun_tool();
  if (p_i->run_size_attr < core_set::get_set().p_max_thread && !p_i->run_process_arg_attr.empty()) {
    auto l_run = p_i->run_process_arg_attr.top();
    p_i->run_process_arg_attr.pop();
    l_run->run();
    p_i->run_attr.emplace_back(l_run);
  }
  /// @brief 清除运行完成的程序
  for (auto &&l_i : p_i->run_attr) {
    if (!l_i->child_attr.running()) {
      boost::asio::post(g_io_context(), [l_i, this]() {
        this->p_i->run_attr |= ranges::action::remove_if([&](auto &&j) -> bool { return l_i == j; });
      });
    }
  }
}
void maya_exe::queue_up(
    const entt::handle &in_msg, const std::string &in_string, const std::shared_ptr<call_fun_type> &in_call_fun,
    const FSys::path &in_run_path
) {
  DOODLE_CHICK(
      core_set::get_set().has_maya(), doodle_error{"没有找到maya路径 (例如 C:/Program Files/Autodesk/Maya2019/bin})"}
  );
  auto l_run             = p_i->run_process_arg_attr.emplace(std::make_shared<maya_exe_ns::run_maya>());
  l_run->mag_attr        = in_msg;
  l_run->run_script_attr = in_string;
  l_run->file_path_attr  = in_run_path;
  l_run->call_attr =
      std::make_shared<call_fun_type>([in_call_fun, this, in_msg](const boost::system::error_code &in_code) {
        boost::asio::post(g_io_context(), [=]() {
          auto &&l_msg = in_msg.get<process_message>();
          l_msg.set_state(l_msg.success);
          if (!in_code) {
            l_msg.message("成功完成", l_msg.warning);
          }
          (*in_call_fun)(in_code);
          this->notify_run();
        });
      });
  notify_run();
}
maya_exe::~maya_exe() = default;

}  // namespace doodle
