//
// Created by TD on 2022/5/30.
//

#include "app_base.h"

#include <doodle_core/core/app_facet.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/init_register.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/gui_template/show_windows.h>
#include <doodle_core/logger/logger.h>

#include <boost/asio.hpp>
#include <boost/locale.hpp>

#include "core/app_base.h"
#include <memory>
#include <thread>

namespace doodle {

app_base* app_base::self = nullptr;

app_base::app_base()
    : p_title(boost::locale::conv::utf_to_utf<wchar_t>(fmt::format("doodle {}", version::build_info::get().version_str))
      ),
      stop_(false),
      lib_ptr(std::make_shared<doodle_lib>()) {
  self                   = this;
  auto&& l_program_info  = lib_ptr->ctx().emplace<program_info>();
  l_program_info.handle_ = ::GetModuleHandleW(nullptr);
  init();
  auto l_timer = std::make_shared<boost::asio::high_resolution_timer>(g_io_context());
  l_timer->expires_after(1s);
  l_timer->async_wait([l_timer, this](auto) {
    /// 检查授权失败直接退出
    if (!chick_authorization()) {
      DOODLE_LOG_INFO("授权失败, 退出");
      stop_app();
    }
  });
}

void app_base::init() {
  DOODLE_LOG_INFO("开始初始化基本配置");

  core_set_init k_init{};

  DOODLE_LOG_INFO("寻找用户配置文件目录");
  k_init.config_to_user();
  k_init.find_maya();
  DOODLE_LOG_INFO("读取配置文件");
  k_init.read_file();
  DOODLE_LOG_INFO("寻找到自身exe {}", core_set::get_set().program_location());
  boost::asio::post(g_io_context(), [this]() { this->post_constructor(); });
}
app_base::~app_base() = default;

app_base& app_base::Get() { return *self; }
std::int32_t app_base::run() {
  g_io_context().run();
  return 0;
}

std::int32_t app_base::poll_one() {
  g_io_context().poll_one();
  return 0;
}
void app_base::stop_app(bool in_stop) {
  lib_ptr->ctx().emplace<program_info>().is_stop = true;
  this->deconstruction();
  core_set_init{}.write_file();
  //  boost::asio::post(g_io_context(), [=]() {
  //  });
}

void app_base::load_project(const FSys::path& in_path) const {
  boost::ignore_unused(this);
  if (!in_path.empty() && FSys::exists(in_path) && FSys::is_regular_file(in_path) &&
      in_path.extension() == doodle_config::doodle_db_name.data()) {
    g_reg()->ctx().at<database_n::file_translator_ptr>()->async_open(in_path, [](bsys::error_code) -> void {
      DOODLE_LOG_INFO("完成打开项目");
    });
  }
}

bool app_base::is_main_thread() const { return run_id == std::this_thread::get_id(); }

}  // namespace doodle
