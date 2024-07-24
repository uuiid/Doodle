//
// Created by TD on 2022/6/2.
//

#include "sqlite_client.h"

#include "doodle_core_fwd.h"
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/core_sql.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/database_task/details/load_save_impl.h>
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio.hpp>

#include "configure/static_value.h"
#include "core/file_sys.h"
#include "core/global_function.h"
#include "logger/logger.h"
#include <any>
#include <core/core_set.h>
#include <core/status_info.h>
#include <database_task/select.h>
#include <filesystem>
#include <fmt/core.h>
#include <utility>

namespace doodle::database_n {
file_translator::file_translator()
  : registry_attr(g_reg()), obs(obs_all{}), timer_(std::make_shared<timer_t>(g_io_context())) {
}

file_translator::file_translator(registry_ptr in_registry)
  : registry_attr(in_registry), obs(obs_all{}), timer_(std::make_shared<timer_t>(g_io_context())) {
}

registry_ptr file_translator::load_new_file(const FSys::path& in_path) {
  registry_ptr l_reg = std::make_shared<entt::registry>();
  if (!FSys::exists(in_path)) return l_reg;
  obs_all l_obs{};
  l_reg->ctx().emplace<database_info>().path_ = in_path;
  std::int32_t l_count                        = 0;
  do {
    try {
      auto l_con = l_reg->ctx().get<database_info>().get_connection_const();
      l_obs.open(l_reg, l_con);
      break;
    } catch (const sqlpp::exception& in_error) {
      l_reg->clear();
      std::this_thread::sleep_for(std::chrono::microseconds{1});
      default_logger_raw()->log(log_loc(), level::err, "打开文件 {} 失败 {}", in_path, in_error.what());
    }
    ++l_count;
  } while (l_count < 100);
  if (l_count >= 100) {
    default_logger_raw()->log(log_loc(), level::err, "打开文件 {} 失败", in_path);
    return l_reg;
  }
  l_reg->ctx().emplace<project_config::base_config>(project_config::base_config::get_default());
  l_reg->ctx().emplace<project>("tmp", in_path, "tmp", "tmp", "");
  return l_reg;
}

void file_translator::new_file_scene(const FSys::path& in_path, const project& in_project) {
  auto& l_obs = std::any_cast<obs_all&>(obs);
  l_obs.disconnect();
  l_obs.clear();
  g_ctx().get<database_info>().path_ = in_path;
  g_reg()->clear();
  l_obs.connect(registry_attr);
  g_reg()->ctx().emplace<project>()                     = in_project;
  g_reg()->ctx().emplace<project_config::base_config>() = project_config::base_config::get_default();
  project_path                                          = in_path;
  save_all                                              = false;
  auto& l_s                                             = g_ctx().emplace<status_info>();
  l_s.message                                           = "创建新项目";
  l_s.need_save                                         = true;
}

boost::system::error_code file_translator::async_open_impl() {
  boost::system::error_code l_error_code{};
  if (project_path != memory_data && !FSys::exists(project_path, l_error_code)) {
    if (l_error_code) return l_error_code;
    default_logger_raw()->log(log_loc(), level::info, "文件不存在 {}", project_path);
    g_ctx().get<database_info>().path_ = project_path;
    registry_attr->ctx().get<project>().set_path(project_path.parent_path());
    g_ctx().get<core_sig>().project_begin_open(project_path);
    auto& l_obs = std::any_cast<obs_all&>(obs);
    l_obs.disconnect();
    l_obs.clear();
    g_reg()->clear();
    l_obs.connect(registry_attr);
    if (g_reg()->ctx().contains<project>()) g_reg()->ctx().erase<project>();
    g_reg()->ctx().emplace<project>("tmp", project_path, "tmp", "tmp", "");
    g_reg()->ctx().emplace<project_config::base_config>() = project_config::base_config::get_default();
    save_all                                              = false;
    l_error_code                                          = async_save_impl();
    g_ctx().get<core_sig>().project_end_open();

    return l_error_code;
  }

  {
    g_ctx().get<database_info>().path_ = project_path;
    g_ctx().get<core_sig>().project_begin_open(project_path);
    registry_attr->clear();
  }

  save_all = false;
  database_n::select l_select{};
  auto& l_obs = std::any_cast<obs_all&>(obs);
  for (int l = 0; l < 10; ++l) {
    try {
      auto l_k_con = g_ctx().emplace<database_info>().get_connection_const();
      if (!l_select.is_old(project_path, l_k_con)) {
        if (only_ctx) {
          l_obs.open_ctx(registry_attr, l_k_con);
          break;
        } else {
          l_obs.open(registry_attr, l_k_con);
          break;
        }
      } else {
        l_obs.disconnect();
        l_select(*registry_attr, project_path, l_k_con);
        l_select.patch();
        save_all = true;
        /// 先监听
        l_obs.connect(registry_attr);
      }
    } catch (const sqlpp::exception& in_error) {
      registry_attr->clear();
      std::this_thread::sleep_for(std::chrono::microseconds{1});
      default_logger_raw()->log(
        log_loc(), level::err, "打开文件 {} 开始重试 {} 失败 {}", project_path, l, in_error.what()
      );
    }
  }

  registry_attr->ctx().get<project>().set_path(project_path.parent_path());
  g_ctx().get<core_sig>().project_end_open();
  core_set::get_set().add_recent_project(project_path);
  only_ctx = false;

  if (!only_open) begin_save();
  return l_error_code;
}

void file_translator::begin_save() {
  if (g_ctx().get<program_info>().stop_attr()) {
    default_logger_raw()->log(log_loc(), level::warn, "程序退出");
    return;
  }
  if (timer_->expiry() <= std::chrono::steady_clock::now()) {
    timer_->expires_from_now(std::chrono::seconds(1));
    timer_->async_wait(boost::asio::bind_cancellation_slot(
      app_base::Get().on_cancel.slot(),
      [this](const boost::system::error_code& in_error) {
        if (in_error) {
          log_info(fmt::format("定时器取消 {}", in_error.message()));
          return;
        }
        if (project_path.empty()) {
          return;
        }

        if (!std::any_cast<obs_all&>(obs).has_update()) {
          begin_save();
          return;
        }

        async_save_impl();
      }
    ));
  }
}

boost::system::error_code file_translator::async_save_impl() {
  {
    if (!FSys::folder_is_save(project_path)) {
      log_warn(fmt::format("{} 权限不够, 不保存", project_path));
      return {};
    }
    if (save_all) project_path.replace_filename(fmt::format("{}_v2.doodle_db", project_path.stem().string()));
    DOODLE_LOG_INFO("文件位置 {}", project_path);
    if (auto l_p = project_path.parent_path(); !FSys::exists(l_p) && !l_p.empty()) {
      FSys::create_directories(l_p);
    }
    g_ctx().get<database_info>().path_ = project_path;
    // 提前测试存在
    if (save_all && FSys::exists(project_path)) {
      log_error(fmt::format("{} 已经存在, 不保存", project_path));
      return {};
    }
    if (save_all) log_error(fmt::format("{} 转换旧版数据, 较慢", project_path));
  }
  auto l_k_con = g_ctx().get<database_info>().get_connection();
  try {
    auto l_tx = sqlpp::start_transaction(*l_k_con);
    if (save_all) {
      std::any_cast<obs_all&>(obs).save_all(registry_attr, l_k_con);
      save_all = false;
    } else {
      std::any_cast<obs_all&>(obs).save(registry_attr, l_k_con);
    }
    l_tx.commit();
  } catch (const sqlpp::exception& in_error) {
    log_error(boost::diagnostic_information(in_error));
  }

  g_ctx().get<status_info>().need_save = false;
  core_set::get_set().add_recent_project(project_path);
  begin_save();
  return {};
}

void file_translator::async_import_impl(const FSys::path& in_path) {
  g_ctx().get<database_info>().path_ = in_path.empty() ? FSys::path{database_info::memory_data} : in_path;
  g_ctx().get<core_sig>().project_begin_open(project_path);
  auto l_old = std::make_shared<bool>();
  default_logger_raw()->log(log_loc(), level::warn, "导入数据 {}", in_path);

  auto l_end_call = [this, l_old]() {
    if (*l_old) {
      default_logger_raw()->log(log_loc(), level::warn, "{}, 旧版文件, 不导入", project_path);
      default_logger_raw()->log(log_loc(), level::off, fmt::to_string(process_message::state::fail));
    } else {
      default_logger_raw()->log(log_loc(), level::off, fmt::to_string(process_message::state::success));
    }
    g_reg()->ctx().erase<process_message>();
    g_ctx().get<core_sig>().project_end_open();
  };

  boost::asio::post(
    g_thread(),
    [l_image = in_path.parent_path() / doodle_config::image_folder_name,
      l_target = project_path.parent_path() / doodle_config::image_folder_name]() {
      FSys::copy(l_image, l_target, FSys::copy_options::recursive | FSys::copy_options::overwrite_existing);
    }
  );
  boost::asio::post(
    g_thread(),
    [this, in_path, l_k_con = g_ctx().get<database_info>().get_connection_const(), l_end_call, l_old]() mutable {
      database_n::select l_select{};
      if (l_select.is_old(in_path, l_k_con)) {
        DOODLE_LOG_INFO("旧版文件, 不导入");
        *l_old = true;
      }
      std::any_cast<obs_all&>(obs).import_project(registry_attr, l_k_con);
      boost::asio::post(g_io_context(), l_end_call);
    }
  );
}
} // namespace doodle::database_n