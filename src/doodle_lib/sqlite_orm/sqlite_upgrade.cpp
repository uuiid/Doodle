//
// Created by TD on 25-5-15.
//
//

#include "doodle_core/metadata/entity.h"
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/seedance2/task.h>
#include <doodle_core/metadata/task_type.h>

#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_upgrade.h>

#include "core/core_set.h"
#include "sqlite_orm/orm/alias.h"
#include "sqlite_orm/orm/exception.h"
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace doodle::details {
namespace {
constexpr std::size_t g_current_version = 11;
}

struct upgrade_init_t : sqlite_upgrade {
  explicit upgrade_init_t() {}

  static void full_fts_sync(sqlite_storage& in_data) {
    using namespace orm;
    auto l_s = in_data.create_session();
    try {
      insert(l_s).into<entity_fts>().set(c(any_column<entity_fts>()) = "integrity-check", c(rank()) = 1)();
    } catch (const sqlite_orm_exception& e) {
      SPDLOG_WARN("FTS integrity check failed: {}", e.what());
      insert(l_s).into<entity_fts>().set(c(any_column<entity_fts>()) = "rebuild")();
    }
  }

  void upgrade(sqlite_storage& in_data) override {
    if (in_data.pragma().user_version() != 0) return;
    in_data.sync_schema();
    in_data.pragma().user_version(g_current_version);
    auto l_session = sqlite_database{in_data.create_session()};

#define DOODLE_ASSET_TYPE(class_name)                        \
  for (const auto& v : class_name::get_all_constant()) {     \
    if (l_session.uuid_to_id<class_name>(v.uuid_id_) == 0) { \
      auto l_s = std::make_shared<class_name>(v);            \
      l_session.install_unsafe<class_name>(l_s);             \
    }                                                        \
  }

    DOODLE_ASSET_TYPE(project_status)
    DOODLE_ASSET_TYPE(assets_helper::database_t)
    DOODLE_ASSET_TYPE(asset_type)
    DOODLE_ASSET_TYPE(task_type)

#undef DOODLE_ASSET_TYPE
  }
};  // namespace doodle::details

namespace {
struct entity_asset_extend_old {
  DOODLE_BASE_FIELDS();
  uuid entity_id_;

  std::optional<std::int32_t> ji_shu_lie_;
  std::string deng_ji_;
  std::optional<std::int32_t> gui_dang_;
  std::string bian_hao_;
  std::string pin_yin_ming_cheng_;
  std::string ban_ben_;
  std::optional<std::int32_t> ji_du_;
  std::optional<std::int32_t> kai_shi_ji_shu_;
  std::optional<std::int32_t> chang_ci_{};
};
struct project_data {
  std::vector<std::pair<entity, entity_asset_extend_old>> entity_asset_extends_;
  std::map<std::string, entity> eps_entities_;
  std::vector<entity> assets_entities_;
};

}  // namespace

struct upgrade_2_t : sqlite_upgrade {
  explicit upgrade_2_t() {}
  void upgrade(sqlite_storage& in_data) override {
    upgrade_init_t::full_fts_sync(in_data);
    in_data.pragma().user_version(g_current_version);
  }
  ~upgrade_2_t() override = default;
};

std::shared_ptr<sqlite_upgrade> upgrade_init() { return std::make_shared<upgrade_init_t>(); }
std::shared_ptr<sqlite_upgrade> upgrade_1() { return std::make_shared<upgrade_2_t>(); }

}  // namespace doodle::details