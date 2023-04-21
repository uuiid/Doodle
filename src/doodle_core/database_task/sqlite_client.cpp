//
// Created by TD on 2022/6/2.
//

#include "sqlite_client.h"

#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/core_sql.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/database_task/details/assets.h>
#include <doodle_core/database_task/details/assets_file.h>
#include <doodle_core/database_task/details/comment.h>
#include <doodle_core/database_task/details/database.h>
#include <doodle_core/database_task/details/episodes.h>
#include <doodle_core/database_task/details/export_file_info.h>
#include <doodle_core/database_task/details/image_icon.h>
#include <doodle_core/database_task/details/importance.h>
#include <doodle_core/database_task/details/macro.h>
#include <doodle_core/database_task/details/project.h>
#include <doodle_core/database_task/details/project_config.h>
#include <doodle_core/database_task/details/redirection_path_info.h>
#include <doodle_core/database_task/details/rules.h>
#include <doodle_core/database_task/details/season.h>
#include <doodle_core/database_task/details/shot.h>
#include <doodle_core/database_task/details/time_point_info.h>
#include <doodle_core/database_task/details/time_point_wrap.h>
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/details/user.h>
#include <doodle_core/database_task/details/work_task.h>
#include <doodle_core/gui_template/show_windows.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_task.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "core/core_help_impl.h"
#include "entt/entity/fwd.hpp"
#include "entt/signal/sigh.hpp"
#include "metadata/project.h"
#include "range/v3/algorithm/any_of.hpp"
#include <core/core_set.h>
#include <core/status_info.h>
#include <database_task/details/database.h>
#include <database_task/select.h>
#include <metadata/metadata.h>
#include <range/v3/all.hpp>
#include <utility>

namespace doodle::database_n {

bsys::error_code file_translator::open_begin(const FSys::path& in_path) {
  doodle_lib::Get().ctx().get<database_info>().path_ = in_path;
  auto& k_msg                                        = g_reg()->ctx().emplace<process_message>();
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
  core_set::get_set().add_recent_project(doodle_lib::Get().ctx().get<database_info>().path_);
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
  g_reg()->ctx().get<core_sig>().save_begin();
  is_saving = true;
  return {};
}

bsys::error_code file_translator::save(const FSys::path& in_path) {
  auto l_r = save_impl(in_path);
  return l_r;
}

bsys::error_code file_translator::save_end() {
  g_reg()->ctx().get<status_info>().need_save = false;
  g_reg()->ctx().get<core_sig>().save_end();
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
  doodle_lib::Get().ctx().get<database_info>().path_ = in_path;
  auto& l_s                                          = g_reg()->ctx().emplace<status_info>();
  l_s.message                                        = "创建新项目";
  l_s.need_save                                      = true;
}

class sqlite_file::impl {
 public:
  registry_ptr registry_attr;
  template <typename type_t>
  class impl_obs {
    std::shared_ptr<entt::observer> obs_update_;
    std::shared_ptr<entt::observer> obs_create_;

   public:
    explicit impl_obs(const registry_ptr& in_registry_ptr)
        : obs_update_(
              std::make_shared<entt::observer>(*in_registry_ptr, entt::collector.update<database>().where<type_t>())
          ),
          obs_create_(std::make_shared<entt::observer>(*in_registry_ptr, entt::collector.group<database, type_t>())) {}

    impl_obs(const impl_obs&)            = default;
    impl_obs(impl_obs&&)                 = default;
    impl_obs& operator=(const impl_obs&) = default;
    impl_obs& operator=(impl_obs&&)      = default;

    ~impl_obs()                          = default;
    void open(const registry_ptr& in_registry_ptr, conn_ptr& in_conn, std::map<std::int64_t, entt::entity>& in_handle) {
      obs_update_->disconnect();
      obs_create_->disconnect();
      database_n::sql_com<type_t> l_table{in_registry_ptr};
      if (l_table.has_table(in_conn)) l_table.select(in_conn, in_handle);
      obs_update_->connect(*in_registry_ptr, entt::collector.update<database>().where<type_t>());
      obs_create_->connect(*in_registry_ptr, entt::collector.group<database, type_t>());
    };

    void save(const registry_ptr& in_registry_ptr, conn_ptr& in_conn, const std::vector<std::int64_t>& in_handle) {
      database_n::sql_com<type_t> l_orm{in_registry_ptr};
      l_orm.create_table(in_conn);

      std::vector<entt::entity> l_create{};

      for (auto&& i : *obs_create_) {
        l_create.emplace_back(i);
      }
      for (auto&& i : *obs_update_) {
        l_create.emplace_back(i);
      }

      BOOST_ASSERT(ranges::any_of(l_create, [&](entt::entity& i) {
        return in_registry_ptr->get<database>(i).is_install();
      }));

      l_orm.insert(in_conn, l_create);
      l_orm.destroy(in_conn, in_handle);
      obs_update_->clear();
      obs_create_->clear();
    }
  };

  template <>
  class impl_obs<database> {
    std::shared_ptr<entt::observer> obs_create_;
    std::vector<std::int64_t> destroy_ids_{};
    std::shared_ptr<entt::scoped_connection> conn_{};
    void on_destroy(entt::registry& in_reg, entt::entity in_entt) {
      if (auto& l_data = in_reg.get<database>(in_entt); l_data.is_install()) destroy_ids_.emplace_back(l_data.get_id());
    }

   public:
    explicit impl_obs(const registry_ptr& in_registry_ptr)
        : obs_create_(std::make_shared<entt::observer>(*in_registry_ptr, entt::collector.group<database>())),
          destroy_ids_{},
          conn_{} {
      // std::make_shared<entt::scoped_connection>(
      //               in_registry_ptr->on_destroy<database>().connect<&impl_obs<database>::on_destroy>(*this)
      //           )
      in_registry_ptr->on_destroy<database>().connect<&impl_obs<database>::on_destroy>(*this);
    }
    impl_obs(const impl_obs&)            = default;
    impl_obs(impl_obs&&)                 = default;
    impl_obs& operator=(const impl_obs&) = default;
    impl_obs& operator=(impl_obs&&)      = default;
    ~impl_obs()                          = default;

    void open(const registry_ptr& in_registry_ptr, conn_ptr& in_conn, std::map<std::int64_t, entt::entity>& in_handle) {
      obs_create_->disconnect();
      database_n::sql_com<database> l_table{in_registry_ptr};
      if (l_table.has_table(in_conn)) l_table.select(in_conn, in_handle);
      obs_create_->connect(*in_registry_ptr, entt::collector.group<database>());
      destroy_ids_.clear();
    };

    void save(const registry_ptr& in_registry_ptr, conn_ptr& in_conn, std::vector<std::int64_t>& in_handle) {
      database_n::sql_com<database> l_orm{in_registry_ptr};
      l_orm.create_table(in_conn);

      std::vector<entt::entity> l_create{};

      for (auto&& i : *obs_create_) {
        if (!in_registry_ptr->get<database>(i).is_install()) l_create.emplace_back(i);
      }

      in_handle = destroy_ids_;
      l_orm.insert(in_conn, l_create);
      l_orm.destroy(in_conn, destroy_ids_);
      destroy_ids_.clear();
      obs_create_->clear();
    }
  };

  template <typename... arg>
  class obs_main {
    std::tuple<impl_obs<database>, impl_obs<arg>...> obs_data_;

   public:
    explicit obs_main(const registry_ptr& in_registry_ptr = g_reg())
        : obs_data_{std::move(impl_obs<database>{in_registry_ptr}), std::move(impl_obs<arg>{in_registry_ptr})...} {}

    void open(const registry_ptr& in_registry_ptr, conn_ptr& in_conn) {
      std::map<std::int64_t, entt::entity> l_map{};

      std::apply([&](auto&&... x) { ((x.open(in_registry_ptr, in_conn, l_map), ...)); }, obs_data_);
    }

    void save(const registry_ptr& in_registry_ptr, conn_ptr& in_conn) {
      std::vector<std::int64_t> l_handles{};
      std::apply([&](auto&&... x) { (x.save(in_registry_ptr, in_conn, l_handles), ...); }, obs_data_);
    }
  };

  obs_main<
      doodle::project, doodle::project_config::base_config, doodle::episodes, doodle::shot, doodle::season,
      doodle::assets, doodle::assets_file, doodle::time_point_wrap, doodle::comment, doodle::image_icon,
      doodle::importance, doodle::redirection_path_info, doodle::business::rules, doodle::user, doodle::work_task_info>
      obs_save;
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
  auto l_k_con = doodle_lib::Get().ctx().get<database_info>().get_connection();
  if (!l_select(*ptr->registry_attr, in_path, l_k_con)) ptr->obs_save.open(ptr->registry_attr, l_k_con);

  for (auto&& [e, p] : ptr->registry_attr->view<project>().each()) {
    ptr->registry_attr->ctx().emplace<project>() = p;
  }
  for (auto&& [e, p] : ptr->registry_attr->view<project_config::base_config>().each()) {
    ptr->registry_attr->ctx().emplace<project_config::base_config>() = p;
  }
  ptr->registry_attr->ctx().at<project>().set_path(in_path);

  return {};
}
bsys::error_code sqlite_file::save_impl(const FSys::path& in_path) {
  ptr->registry_attr = g_reg();

  DOODLE_LOG_INFO("文件位置 {}", in_path);
  doodle_lib::Get().ctx().get<database_info>().path_ = in_path;
  try {
    auto l_k_con = doodle_lib::Get().ctx().get<database_info>().get_connection();
    auto l_tx    = sqlpp::start_transaction(*l_k_con);
    ptr->obs_save.save(ptr->registry_attr, l_k_con);
    l_tx.commit();
  } catch (const sqlpp::exception& in_error) {
    DOODLE_LOG_INFO(boost::diagnostic_information(in_error));
    g_reg()->ctx().at<status_info>().message = "保存失败 3s 后重试";
    ptr->error_timer                         = std::make_shared<boost::asio::system_timer>(g_io_context());
    ptr->error_timer->async_wait([l_path = in_path, this](auto&& in) {
      this->async_save(l_path, [](boost::system::error_code in) -> void {});
    });
    ptr->error_timer->expires_from_now(3s);
  }

  return {};
}

sqlite_file::~sqlite_file()                                    = default;
sqlite_file::sqlite_file(sqlite_file&& in) noexcept            = default;
sqlite_file& sqlite_file::operator=(sqlite_file&& in) noexcept = default;

}  // namespace doodle::database_n
