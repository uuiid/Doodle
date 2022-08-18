//
// Created by TD on 2022/6/2.
//

#include "sqlite_client.h"

#include <database_task/insert.h>
#include <database_task/select.h>
#include <database_task/update.h>
#include <database_task/delete_data.h>
#include <database_task/details/update_ctx.h>

#include <thread_pool/process_pool.h>
#include <doodle_core/core/core_sig.h>
#include <metadata/metadata.h>

#include <range/v3/all.hpp>
#include <core/status_info.h>

#include <boost/ptr_container/ptr_vector.hpp>
#include <utility>
#include <core/core_set.h>
#include <doodle_core/thread_pool/process_message.h>
namespace doodle::database_n {

void sqlite_client::open_sqlite(const FSys::path& in_path, bool only_ctx) {
  g_reg()->ctx().at<core_sig>().project_begin_open(in_path);
  boost::asio::post(
      make_process_adapter<database_n::select>(
          g_io_context().get_executor(), database_n::select::arg{in_path, only_ctx})
          .next([]() {
            g_reg()->ctx().at<core_sig>().project_end_open();
          }));
}

void sqlite_client::update_entt() {
  std::vector<entt::entity> delete_list;
  std::vector<entt::entity> all_list;
  std::vector<entt::entity> install_list;
  std::vector<entt::entity> update_list;
  std::vector<entt::entity> next_delete_list;

  auto l_dv = g_reg()->view<data_status_delete, database>();
  for (auto&& [e, d] : l_dv.each()) {
    if (d.is_install()) {
      delete_list.push_back(e);
    } else {
      next_delete_list.push_back(e);
    }
  }

  auto l_sv = g_reg()->view<data_status_save, database>();
  for (auto&& [e, d] : l_sv.each()) {
    if (d.is_install()) {
      update_list.push_back(e);
    } else {
      install_list.push_back(e);
    }
  }
  all_list |= ranges::actions::push_back(delete_list) |
              ranges::actions::push_back(install_list) |
              ranges::actions::push_back(update_list) |
              ranges::actions::push_back(next_delete_list) |
              ranges::actions::unique;

  if (all_list.empty()) {
    /// \brief 只更新上下文
    auto l_s = boost::asio::make_strand(g_io_context());
    boost::asio::post(l_s, [l_s]() {
      g_reg()->ctx().at<core_sig>().save_begin({});
    });
    boost::asio::post(l_s, [l_s]() {
      database_n::details::update_ctx::ctx(*g_reg());
    });
    boost::asio::post(l_s, [l_s]() {
      g_reg()->ctx().at<core_sig>().save_end({});
    });

    return;
  }

  auto l_list = all_list | ranges::view::transform([](auto e) {
                  return make_handle(e);
                }) |
                ranges::to_vector;
  auto l_next = make_process_adapter(g_io_context().get_executor(), [=]() {
    g_reg()->ctx().at<core_sig>().save_begin(l_list);
    /// \brief 删除没有插入的
    g_reg()->destroy(next_delete_list.begin(), next_delete_list.end());
  });

  if (!install_list.empty()) {
    l_next.next<database_n::insert>(install_list);
  }
  if (!update_list.empty()) {
    l_next.next<database_n::update_data>(update_list);
  }
  if (!delete_list.empty()) {
    l_next.next<database_n::delete_data>(delete_list);
  }
  l_next.next([=]() {
    auto l_sv = g_reg()->view<data_status_save>();
    g_reg()->remove<data_status_save>(l_sv.begin(), l_sv.end());
    auto l_dv = g_reg()->view<data_status_delete>();
    g_reg()->remove<data_status_save>(l_dv.begin(), l_dv.end());
    g_reg()->ctx().at<core_sig>().save_end(l_list);

    g_reg()->ctx().at<status_info>().need_save = false;
  });
  boost::asio::post(l_next);
}
void sqlite_client::create_sqlite() {
  boost::asio::post(
      make_process_adapter<database_n::insert>(g_io_context().get_executor(), std::vector<entt::entity>{})
          .next<database_n::update_data>(std::vector<entt::entity>{}));
}

bsys::error_code file_translator::open(const FSys::path& in_path) {
  g_reg()->ctx().at<::doodle::database_info>().path_ = in_path;
  g_reg()->clear();
  g_reg()
      ->ctx()
      .at<core_sig>()
      .project_begin_open(in_path);
  return open_impl(in_path);
}

bsys::error_code file_translator::open_end() {
  core_set::getSet().add_recent_project(g_reg()->ctx().at<::doodle::database_info>().path_);
  g_reg()->ctx().at<core_sig>().project_end_open();
  return {};
}

bsys::error_code file_translator::save(const FSys::path& in_path) {
  auto& k_msg = g_reg()->ctx().emplace<process_message>();
  k_msg.set_name("保存数据");
  k_msg.set_state(k_msg.run);
  return save_impl(in_path);
}

bsys::error_code file_translator::save_end() {
  g_reg()->ctx().at<status_info>().need_save = false;
  auto& k_msg = g_reg()->ctx().emplace<process_message>();
  k_msg.set_name("完成写入数据");
  k_msg.set_state(k_msg.success);
  return {};
}

class sqlite_file::impl {
 public:
  registry_ptr registry_attr;
};

sqlite_file::sqlite_file()
    : sqlite_file(g_reg()) {
}
sqlite_file::sqlite_file(registry_ptr in_registry)
    : ptr(std::make_unique<impl>()) {
  ptr->registry_attr = std::move(in_registry);
}
bsys::error_code sqlite_file::open_impl(const FSys::path& in_path) {
  constexpr auto l_loc = BOOST_CURRENT_LOCATION;
  if (FSys::exists(in_path)) return bsys::error_code{error_enum::file_not_exists, &l_loc};

  database_n::select l_select{};
  l_select(*ptr->registry_attr, in_path);
  return {};
}
bsys::error_code sqlite_file::save_impl(const FSys::path& in_path) {
  if (!FSys::exists(in_path)) {  /// \brief  不存在时直接保存所有的实体
    insert l_insert{};
    auto l_view = g_reg()->view<doodle::database>();
    l_insert(*g_reg(), std::vector<entt::entity>{l_view.begin(), l_view.end()});
  } else {  /// \brief   否则进行筛选
    std::vector<entt::entity> all_list;

    std::vector<entt::entity> delete_list;
    std::vector<entt::entity> install_list;
    std::vector<entt::entity> update_list;
    std::vector<entt::entity> next_delete_list;

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
    all_list |= ranges::actions::push_back(delete_list) |
                ranges::actions::push_back(install_list) |
                ranges::actions::push_back(update_list) |
                ranges::actions::push_back(next_delete_list) |
                ranges::actions::unique;

    if (all_list.empty()) {
      /// \brief 只更新上下文
      auto l_s = boost::asio::make_strand(g_io_context());
      ptr->registry_attr->ctx().at<core_sig>().save_begin({});
      database_n::details::update_ctx::ctx(*ptr->registry_attr);
      ptr->registry_attr->ctx().at<core_sig>().save_end({});
      return {};
    } else {
      auto l_list = all_list | ranges::view::transform([](auto e) {
                      return make_handle(e);
                    }) |
                    ranges::to_vector;
      ptr->registry_attr->ctx().at<core_sig>().save_begin(l_list);
      /// \brief 删除没有插入的
      ptr->registry_attr->destroy(next_delete_list.begin(), next_delete_list.end());

      if (!install_list.empty()) {
        database_n::insert l_sqlit_action{};
        l_sqlit_action(*ptr->registry_attr, install_list);
      }
      if (!update_list.empty()) {
        database_n::update_data l_sqlit_action{};
        l_sqlit_action(*ptr->registry_attr, update_list);
      }
      if (!delete_list.empty()) {
        database_n::delete_data l_sqlit_action{};
        l_sqlit_action(*ptr->registry_attr, delete_list);
      }
      ptr->registry_attr->clear<data_status_save>();
      ptr->registry_attr->clear<data_status_save>();
      ptr->registry_attr->ctx().at<core_sig>().save_end(l_list);
      ptr->registry_attr->ctx().at<status_info>().need_save = false;
    }
  }

  return {};
}

sqlite_file::~sqlite_file()                                    = default;
sqlite_file::sqlite_file(sqlite_file&& in) noexcept            = default;
sqlite_file& sqlite_file::operator=(sqlite_file&& in) noexcept = default;

}  // namespace doodle::database_n
