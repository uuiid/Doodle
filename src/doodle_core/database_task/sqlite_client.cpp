//
// Created by TD on 2022/6/2.
//

#include "sqlite_client.h"

#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/core_sql.h>
#include <doodle_core/gui_template/show_windows.h>
#include <doodle_core/metadata/work_task.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "core/core_help_impl.h"
#include <core/core_set.h>
#include <core/status_info.h>
#include <database_task/delete_data.h>
#include <database_task/details/update_ctx.h>
#include <database_task/insert.h>
#include <database_task/select.h>
#include <database_task/update.h>
#include <metadata/metadata.h>
#include <range/v3/all.hpp>
#include <utility>

namespace doodle::database_n {

bsys::error_code file_translator::open_begin(const FSys::path& in_path) {
  database_info::value().path_ = in_path;
  auto& k_msg = g_reg()->ctx().emplace<process_message>();
  k_msg.set_name("加载数据");
  k_msg.set_state(k_msg.run);
  g_reg()->ctx().at<core_sig>().project_begin_open(in_path);
  is_opening = true;
  return {};
}
bsys::error_code file_translator::open(const FSys::path& in_path) {
  auto l_r = open_impl(in_path);
  return l_r;
}

bsys::error_code file_translator::open_end() {
  core_set::get_set().add_recent_project(database_info::value().path_);
  g_reg()->ctx().at<core_sig>().project_end_open();
  auto& k_msg = g_reg()->ctx().emplace<process_message>();
  k_msg.set_name("完成写入数据");
  k_msg.set_state(k_msg.success);
  g_reg()->ctx().erase<process_message>();
  is_opening = false;
  return {};
}

bsys::error_code file_translator::save_begin(const FSys::path& in_path) {
  auto& k_msg = g_reg()->ctx().emplace<process_message>();
  k_msg.set_name("保存数据");
  k_msg.set_state(k_msg.run);
  g_reg()->ctx().at<core_sig>().save_begin();
  is_saving = true;
  return {};
}

bsys::error_code file_translator::save(const FSys::path& in_path) {
  auto l_r = save_impl(in_path);
  return l_r;
}

bsys::error_code file_translator::save_end() {
  g_reg()->ctx().at<status_info>().need_save = false;
  g_reg()->ctx().at<core_sig>().save_end();
  g_reg()->clear<data_status_save>();
  g_reg()->clear<data_status_delete>();
  auto& k_msg = g_reg()->ctx().emplace<process_message>();
  k_msg.set_name("完成写入数据");
  k_msg.set_state(k_msg.success);
  g_reg()->ctx().erase<process_message>();
  is_saving = false;
  return {};
}
void file_translator::new_file_scene(const FSys::path& in_path) {
  database_info::value().path_ = in_path;
  auto& l_s     = g_reg()->ctx().emplace<status_info>();
  l_s.message   = "创建新项目";
  l_s.need_save = true;
}

class sqlite_file::impl {
 public:
  registry_ptr registry_attr;
  bool error_retry{false};

  std::shared_ptr<boost::asio::system_timer> error_timer{};
};

sqlite_file::sqlite_file() : ptr(std::make_unique<impl>()) {}
sqlite_file::sqlite_file(registry_ptr in_registry) : ptr(std::make_unique<impl>()) {
  ptr->registry_attr = std::move(in_registry);
}
bsys::error_code sqlite_file::open_impl(const FSys::path& in_path) {
  ptr->registry_attr   = g_reg();
  constexpr auto l_loc = BOOST_CURRENT_LOCATION;
  if (!FSys::exists(in_path)) return bsys::error_code{error_enum::file_not_exists, &l_loc};

  database_n::select l_select{};
  auto l_k_con = database_info::value().get_connection_const();
  l_select(*ptr->registry_attr, in_path, l_k_con);
  return {};
}
bsys::error_code sqlite_file::save_impl(const FSys::path& in_path) {
  ptr->registry_attr = g_reg();
  std::vector<entt::entity> delete_list;
  std::vector<entt::entity> install_list;
  std::vector<entt::entity> update_list;
  std::vector<entt::entity> next_delete_list;
  DOODLE_LOG_INFO("文件位置 {}", in_path);
  if (!FSys::exists(in_path) && in_path.generic_string() != ":memory:"s) {  /// \brief  不存在时直接保存所有的实体
    if (!FSys::exists(in_path.parent_path())) {
      FSys::create_directories(in_path.parent_path());
    }
    auto l_view  = g_reg()->view<doodle::database>();
    install_list = {l_view.begin(), l_view.end()};
  } else {  /// \brief   否则进行筛选

    auto l_dv = ptr->registry_attr->view<data_status_delete, database>();
    for (auto&& [e, d] : l_dv.each()) {
      if (d.is_install()) {
        delete_list.push_back(e);
      } else {
        next_delete_list.push_back(e);
      }
    }

    auto l_sv = ptr->registry_attr->view<data_status_save, database>();
    for (auto&& [e, d] : l_sv.each()) {
      if (d.is_install()) {
        update_list.push_back(e);
      } else {
        install_list.push_back(e);
      }
    }
  }

  try {
    auto l_k_con = database_info ::value().get_connection();
    auto l_tx    = sqlpp::start_transaction(*l_k_con);
    details::db_compatible::add_entity_table(*l_k_con);
    details::db_compatible::add_ctx_table(*l_k_con);
    details::db_compatible::add_component_table(*l_k_con);
    details::db_compatible::add_version_table(*l_k_con);
    details::db_compatible::delete_metadatatab_table(*l_k_con);
    if (delete_list.empty() && install_list.empty() && update_list.empty()) {
      /// \brief 只更新上下文
      database_n::details::update_ctx::ctx(*ptr->registry_attr, *l_k_con);
    } else {
      /// \brief 删除没有插入的
      ptr->registry_attr->destroy(next_delete_list.begin(), next_delete_list.end());
      if (!install_list.empty()) {
        database_n::insert l_sqlit_action{};
        l_sqlit_action(*ptr->registry_attr, install_list, l_k_con);
      }
      if (!update_list.empty()) {
        database_n::update_data l_sqlit_action{};
        l_sqlit_action(*ptr->registry_attr, update_list, l_k_con);
      }
      if (!delete_list.empty()) {
        database_n::delete_data l_sqlit_action{};
        l_sqlit_action(*ptr->registry_attr, delete_list, l_k_con);
      }
    }
    l_tx.commit();
  } catch (const sqlpp::exception& in_error) {
    DOODLE_LOG_INFO(boost::diagnostic_information(in_error));
    auto l_journal_file{in_path};
    l_journal_file += "-journal";
    if (FSys::exists(l_journal_file)) try {
        FSys::remove(l_journal_file);
      } catch (const FSys::filesystem_error& in_error2) {
        DOODLE_LOG_INFO("无法删除数据库日志文件 {}", boost::diagnostic_information(in_error2));
      }
    if (ptr->error_retry) {  /// 重试时不进行下一步重试
      g_reg()->ctx().at<status_info>().message = "重试失败, 不保存";
      ptr->error_retry                         = false;
    } else {
      g_reg()->ctx().at<status_info>().message = "保存失败 3s 后重试";
      ptr->error_retry                         = true;
      ptr->error_timer                         = std::make_shared<boost::asio::system_timer>(g_io_context());
      ptr->error_timer->async_wait([l_path = in_path, this](auto&& in) {
        this->async_save(l_path, [](boost::system::error_code in) -> void {});
      });
      ptr->error_timer->expires_from_now(3s);
    }
  }

  return {};
}

sqlite_file::~sqlite_file()                                    = default;
sqlite_file::sqlite_file(sqlite_file&& in) noexcept            = default;
sqlite_file& sqlite_file::operator=(sqlite_file&& in) noexcept = default;

}  // namespace doodle::database_n
