//
// Created by TD on 2022/6/2.
//

#include "sqlite_client.h"

#include "doodle_core_fwd.h"
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/core_sql.h>
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
    : registry_attr(g_reg()), obs(obs_all{}), timer_(std::make_shared<timer_t>(g_io_context())) {}
file_translator::file_translator(registry_ptr in_registry)
    : registry_attr(in_registry), obs(obs_all{}), timer_(std::make_shared<timer_t>(g_io_context())) {}

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
  auto& l_s                                             = registry_attr->ctx().emplace<status_info>();
  l_s.message                                           = "创建新项目";
  l_s.need_save                                         = true;
}

void file_translator::async_open_impl(const FSys::path& in_path) {
  if (!in_path.empty() && !FSys::exists(in_path)) return;

  project_path = in_path.empty() ? FSys::path{database_info::memory_data} : in_path;
  {
    g_ctx().get<database_info>().path_ = project_path;
    auto& k_msg                        = g_reg()->ctx().emplace<process_message>();
    k_msg.set_name("加载数据");
    k_msg.set_state(k_msg.run);
    g_ctx().get<core_sig>().project_begin_open(project_path);
    registry_attr = g_reg();
    registry_attr->clear();
  }

  save_all = false;
  database_n::select l_select{};
  auto& l_obs  = std::any_cast<obs_all&>(obs);
  auto l_k_con = g_ctx().emplace<database_info>().get_connection_const();
  if (!l_select.is_old(project_path, l_k_con)) {
    if (only_ctx) {
      l_obs.open_ctx(registry_attr, l_k_con);
    } else {
      l_obs.open(registry_attr, l_k_con);
    }

  } else {
    l_obs.disconnect();
    l_select(*registry_attr, project_path, l_k_con);
    l_select.patch();
    save_all = true;
    /// 先监听
    l_obs.connect(registry_attr);
  }

  registry_attr->ctx().get<project>().set_path(project_path.parent_path());
  g_ctx().get<core_sig>().project_end_open();
  auto& k_msg = g_reg()->ctx().emplace<process_message>();
  k_msg.set_name("完成写入数据");
  k_msg.set_state(k_msg.success);
  g_reg()->ctx().erase<process_message>();
  core_set::get_set().add_recent_project(project_path);
  only_ctx = false;

  if (!only_open) begin_save();
}

void file_translator::begin_save() {
  if (timer_->expiry() <= std::chrono::steady_clock::now()) {
    timer_->expires_from_now(std::chrono::seconds(1));
    timer_->async_wait(boost::asio::bind_cancellation_slot(
        app_base::Get().on_cancel.slot(),
        [this](const boost::system::error_code& in_error) {
          if (in_error) {
            log_info(fmt::format("定时器取消 {}", in_error.message()));
            return;
          }
          async_save_impl();
        }
    ));
  }
}

void file_translator::async_save_impl() {
  if (!std::any_cast<obs_all&>(obs).has_update()) {
    begin_save();
    return;
  }

  if (project_path.empty()) {
    begin_save();
    return;
  }

  {
    if (project_path != "") {
      if (!FSys::folder_is_save(project_path)) {
        log_info(fmt::format("{} 权限不够, 不保存", project_path));
        begin_save();
        return;
      }
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
      begin_save();
      return;
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

  g_reg()->ctx().get<status_info>().need_save = false;
  core_set::get_set().add_recent_project(project_path);
  begin_save();
}

void file_translator::async_import_impl(const FSys::path& in_path) {
  {
    g_ctx().get<database_info>().path_ = in_path.empty() ? FSys::path{database_info::memory_data} : in_path;
    auto& k_msg                        = g_reg()->ctx().emplace<process_message>();
    k_msg.set_name("导入数据");
    k_msg.set_state(k_msg.run);
    g_ctx().get<core_sig>().project_begin_open(project_path);
  }
  auto l_old      = std::make_shared<bool>();
  auto l_end_call = [this, l_old]() {
    auto& k_msg = g_reg()->ctx().emplace<process_message>();
    if (*l_old) {
      k_msg.message(fmt::format("{}, 旧版文件, 不导入", project_path));
      k_msg.set_state(k_msg.fail);
    } else {
      k_msg.set_name("完成导入数据");
      k_msg.set_state(k_msg.success);
    }
    g_reg()->ctx().erase<process_message>();
    g_ctx().get<core_sig>().project_end_open();
  };
  boost::asio::post(
      g_thread(),
      [l_image  = in_path.parent_path() / doodle_config::image_folder_name,
       l_target = project_path.parent_path() / doodle_config::image_folder_name]() mutable {
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

}  // namespace doodle::database_n
