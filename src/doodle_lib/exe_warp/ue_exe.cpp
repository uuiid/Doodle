//
// Created by td_main on 2023/7/26.
//

#include "ue_exe.h"

#include "doodle_core/core/global_function.h"
#include "doodle_core/exception/exception.h"
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/thread_copy_io.h>
#include <doodle_lib/toolkit/toolkit.h>

#include "boost/asio/read.hpp"
#include "boost/asio/readable_pipe.hpp"
#include "boost/locale/encoding.hpp"
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/process/windows.hpp>
#include <boost/signals2.hpp>
#include <boost/system.hpp>

#include "fmt/core.h"
#include <filesystem>
#include <memory>
#include <string>
namespace doodle {

class ue_exe::run_ue_base {
 public:
  virtual void run()        = 0;
  virtual void cancel()     = 0;
  virtual bool is_running() = 0;
};

class ue_exe::run_ue : public std::enable_shared_from_this<ue_exe::run_ue>, public run_ue_base {
  boost::process::async_pipe out_attr{g_io_context()};
  boost::process::async_pipe err_attr{g_io_context()};

  boost::asio::streambuf out_strbuff_attr{};
  boost::asio::streambuf err_strbuff_attr{};

 public:
  boost::signals2::scoped_connection cancel_attr{};
  boost::process::child child_attr{};
  ue_exe::call_fun_type call_attr{};
  FSys::path ue_path{};
  std::string arg_attr{};
  logger_ptr logger_attr{};
  ue_exe *self_{};
  bool is_cancel{};

  boost::asio::high_resolution_timer timer_attr{g_io_context()};

  boost::system::error_code chick_ue_plug() {
    // l_doodle_path =D:\Program Files\Epic Games\UE_5.2\Engine\Plugins\Doodle\Doodle.uplugin
    logger_attr->log(log_loc(), level::info, "检查Doodle插件版本");
    auto l_doodle_path = core_set::get_set().ue4_path / "Engine" / "Plugins" / "Doodle" / "Doodle.uplugin";
    std::string l_version{};
    if (FSys::exists(l_doodle_path)) {
      auto l_json = nlohmann::json::parse(FSys::ifstream{l_doodle_path});
      if (l_json.contains("VersionName")) l_version = l_json["VersionName"].get<std::string>();
    }
    if (l_version != version::build_info::get().version_str) {
      logger_attr->log(
          log_loc(), level::warn, "Doodle 插件版本不匹配, 重新安装查插件版本 {} != {}", l_version,
          version::build_info::get().version_str
      );
      try {
        toolkit::installUePath(core_set::get_set().ue4_path / "Engine");
      } catch (const FSys::filesystem_error &error) {
        logger_attr->log(log_loc(), level::err, "安装插件失败 {}", error.what());
        return error.code();
      }
    } else {
      logger_attr->log(
          log_loc(), level::info, "Doodle 插件版本匹配 {} == {}", l_version, version::build_info::get().version_str
      );
    }
    return {};
  }

  virtual void run() override {
    if (is_cancel) {
      logger_attr->log(log_loc(), level::err, "用户结束 ue_exe: {}", ue_path);
      boost::system::error_code l_ec{boost::asio::error::operation_aborted};
      BOOST_ASIO_ERROR_LOCATION(l_ec);
      call_attr->ec_ = l_ec;
      call_attr->complete();
      self_->notify_run();
      return;
    }
    if (auto l_err = chick_ue_plug(); l_err) {
      call_attr->ec_ = l_err;
      call_attr->complete();
      self_->notify_run();
      return;
    }

    if (ue_path.empty() || !FSys::exists(ue_path)) throw_exception(doodle_error{"ue_exe path is empty or not exists"});
    logger_attr->log(log_loc(), level::info, "开始运行 ue_exe: {} {}", ue_path, arg_attr);

    ue_path                           = ue_path.lexically_normal();
    boost::process::environment l_eve = boost::this_process::environment();
    l_eve["UE-LocalDataCachePath"]    = "%GAMEDIR%DerivedDataCache";
    l_eve["UE-SharedDataCachePath"]   = fmt::format("{}\\UE\\DerivedDataCache", core_set::get_set().depot_ip);

    timer_attr.expires_after(chrono::seconds{core_set::get_set().timeout * 5});
    timer_attr.async_wait([this](boost::system::error_code in_code) {
      if (!in_code) {
        logger_attr->log(log_loc(), level::warn, "进程超时，结束任务");
        child_attr.terminate();
      } else {
        if (in_code != boost::asio::error::operation_aborted) logger_attr->log(log_loc(), level::warn, in_code);
      }
    });

    child_attr = boost::process::child{
        g_io_context(),
        //        boost::process::exe  = ue_path.generic_string(),
        //        boost::process::args = arg_attr,
        boost::process::cmd = fmt::format("{} {}", ue_path, arg_attr), boost::process::std_out > out_attr,
        boost::process::std_err > err_attr,
        boost::process::on_exit =
            [this, l_self = shared_from_this()](int in_exit, const std::error_code &in_error_code) {
              logger_attr->log(log_loc(), level::info, "运行结束 ue_exe: {} 退出代码 {}", ue_path, in_exit);
              logger_attr->log(log_loc(), level::off, magic_enum::enum_name(process_message::state::pause));
              
              if (in_exit != 0 && !in_error_code) {
                boost::system::error_code l_ec{boost::system::errc::make_error_code(boost::system::errc::io_error)};
                BOOST_ASIO_ERROR_LOCATION(l_ec);
                call_attr->ec_ = l_ec;
              } else {
                call_attr->ec_ = in_error_code;
              }
              call_attr->complete();
              self_->notify_run();
            },
        boost::process::windows::hide, l_eve
    };

    read_out();
    read_err();
  }
  void read_out() {
    boost::asio::async_read_until(
        out_attr, out_strbuff_attr, '\n',
        [this, l_self = shared_from_this()](boost::system::error_code in_code, std::size_t in_n) {
          if (!in_code) {
            std::string l_line;
            std::istream l_istream{&out_strbuff_attr};
            std::getline(l_istream, l_line);
            while (!l_line.empty() && std::iscntrl(l_line.back(), core_set::get_set().utf8_locale)) l_line.pop_back();
            if (!l_line.empty()) logger_attr->log(log_loc(), level::debug, l_line);
            read_out();
          } else {
            out_attr.close();
            if (in_code != boost::asio::error::broken_pipe)
              logger_attr->log(log_loc(), level::err, "管道错误 ue_exe: {} {}", ue_path, in_code);
          }
        }
    );
  }
  void read_err() {
    boost::asio::async_read_until(
        err_attr, err_strbuff_attr, '\n',
        [this, l_self = shared_from_this()](boost::system::error_code in_code, std::size_t in_n) {
          if (!in_code) {
            std::string l_line{};
            std::istream l_istream{&err_strbuff_attr};
            std::getline(l_istream, l_line);
            while (!l_line.empty() && std::iscntrl(l_line.back(), core_set::get_set().utf8_locale)) l_line.pop_back();
            if (!l_line.empty()) logger_attr->log(log_loc(), level::info, l_line);
            read_err();
          } else {
            err_attr.close();
            if (in_code != boost::asio::error::broken_pipe)
              logger_attr->log(log_loc(), level::err, "管道错误 ue_exe: {} {}", ue_path, in_code);
          }
        }
    );
  }
  bool is_running() override { return child_attr.running(); }
  virtual void cancel() override {
    child_attr.terminate();
    is_cancel = true;
  }
};

class ue_exe::run_ue_copy_file : public ue_exe::run_ue_base {
 public:
  std::vector<std::pair<FSys::path, FSys::path>> copy_path_attr{};
  ue_exe::call_fun_type call_attr{};
  logger_ptr logger_attr{};
  void run() override {
    g_ctx().get<thread_copy_io_service>().async_copy_old(
        copy_path_attr, FSys::copy_options::recursive, logger_attr,
        [l_c = call_attr, logger_attr = logger_attr](boost::system::error_code in_error_code) {
          if (in_error_code) {
            BOOST_ASIO_ERROR_LOCATION(in_error_code);
          }
          logger_attr->log(log_loc(), level::off, magic_enum::enum_name(process_message::state::pause));
          l_c->ec_ = in_error_code;
          l_c->complete();
        }
    );
  }
  void cancel() override {}
  bool is_running() override { return *call_attr; }
};

void ue_exe::notify_run() {
  if (run_process_ && !run_process_->is_running()) run_process_.reset();

  if (!g_ctx().get<program_info>().stop_attr()) {
    if (!queue_list_.empty() && !run_process_) {
      run_process_ = queue_list_.top();
      queue_list_.pop();
      run_process_->run();
    }
  }
}

void ue_exe::queue_up(const entt::handle &in_msg, const std::string &in_command_line, call_fun_type in_call_fun) {
  auto l_run         = std::make_shared<run_ue>();
  l_run->ue_path     = ue_path_;
  l_run->arg_attr    = in_command_line;
  l_run->call_attr   = std::move(in_call_fun);
  l_run->self_       = this;
  l_run->logger_attr = in_msg.patch<process_message>().logger();
  l_run->cancel_attr =
      in_msg.patch<process_message>().aborted_sig.connect([l_run_weak_ptr = l_run->weak_from_this()]() {
        if (auto l_ptr = l_run_weak_ptr.lock(); l_ptr) {
          l_ptr->cancel();
        }
      });
  queue_list_.emplace(l_run);

  notify_run();
}

void ue_exe::queue_up(
    const entt::handle &in_msg, const std::vector<std::pair<FSys::path, FSys::path>> &in_command_line,
    doodle::ue_exe::call_fun_type in_call_fun
) {
  if (!g_ctx().contains<thread_copy_io_service>()) g_ctx().emplace<thread_copy_io_service>();

  auto l_run            = std::make_shared<run_ue_copy_file>();
  l_run->copy_path_attr = in_command_line;
  l_run->call_attr      = std::move(in_call_fun);
  l_run->logger_attr    = in_msg.patch<process_message>().logger();
  queue_list_.emplace(l_run);
  notify_run();
}

boost::system::error_code ue_exe::find_ue_exe(const logger_ptr &in_logger) {
  auto l_ue_path = core_set::get_set().ue4_path;
  if (l_ue_path.empty()) {
    in_logger->log(log_loc(), level::err, "ue_exe 路径为空, 无法启动UE");
    return boost::system::error_code{boost::system::errc::no_such_file_or_directory, boost::system::system_category()};
  }
  ue_path_ = l_ue_path / doodle_config::ue_path_obj;

  if (!FSys::exists(ue_path_)) {
    return boost::system::error_code{boost::system::errc::no_such_file_or_directory, boost::system::system_category()};
  }
  return {};
}
std::string ue_exe::get_file_version(const FSys::path &in_path) {
  auto l_version_path = in_path.parent_path() / "UnrealEditor.version";

  if (!FSys::exists(l_version_path)) {
    throw_exception(doodle_error{"未找到 UnrealEditor.version 文件"});
  }
  FSys::ifstream l_ifstream{l_version_path};
  nlohmann::json const l_json = nlohmann::json::parse(l_ifstream);

  return fmt::format("{}.{}", l_json["MajorVersion"].get<std::int32_t>(), l_json["MinorVersion"].get<std::int32_t>());
}

}  // namespace doodle