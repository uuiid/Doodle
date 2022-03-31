//
// Created by TD on 2022/1/13.
//

#include "client.h"

#include <doodle_lib/core/core_sql.h>
#include <doodle_lib/long_task/process_pool.h>
#include <doodle_lib/long_task/database_task.h>
#include <doodle_lib/thread_pool/process_message.h>
#include <doodle_lib/metadata/metadata.h>
#include <doodle_lib/core/core_sig.h>
#include <doodle_lib/metadata/project.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/ppgen.h>

#pragma warning(disable : 4003)
// clang-format off
SQLPP_DECLARE_TABLE(
    (doodle_info),
    (version_major, int, SQLPP_NULL)
    (version_minor, int, SQLPP_NULL)
    );
// clang-format on
#pragma warning(default : 4003)

namespace doodle::core {
class client::impl {
 public:
  impl() : data_path() {}
  FSys::path data_path;

  void add_trigger() const {
    auto k_conn = core_sql::Get().get_connection(data_path);
    k_conn->execute(R"(
create trigger if not exists UpdataLastTime AFTER UPDATE OF user_data,uuidPath,parent
    ON metadatatab
begin
    update metadatatab set update_time =CURRENT_TIMESTAMP where id = old.id;
end;
)");
  };
  void add_uuid_row() const {
    auto k_conn = core_sql::Get().get_connection(data_path);
    k_conn->execute(
        R"(alter table metadatatab
add uuid_data text;)");
  }

  std::tuple<std::uint32_t, std::uint32_t> get_version() {
    auto k_con = core_sql::Get().get_connection_const(data_path);
    doodle_info::doodle_info l_info{};

    for (auto&& row : (*k_con)(
             sqlpp::select(all_of(l_info)).from(l_info).unconditionally())) {
      return std::make_tuple(row.version_major.value(),
                             row.version_minor.value());
    }
    chick_true<doodle_error>(false, DOODLE_LOC, "无法检查到数据库 路径{}", data_path);
    return {};
  }

  void set_version() const {
    auto k_conn = core_sql::Get().get_connection(data_path);
    doodle_info::doodle_info l_info{};

    (*k_conn)(sqlpp::update(l_info).unconditionally().set(
        l_info.version_major = version::version_major,
        l_info.version_minor = version::version_minor));
  }

  void up_data() {
    auto [l_main_v, l_s_v] = get_version();
    switch (l_main_v) {
      case 3: {
        switch (l_s_v) {
          case 3: {
            add_trigger();
            add_uuid_row();
          };
          case 4: {
          };
        }
      };
      default:
        break;
    }

    if (l_main_v != version::version_major || l_s_v != version::version_minor)
      set_version();
  };
};

client::client()
    : p_i(std::make_unique<impl>()) {
  p_i->data_path = "null";
}
client::~client() = default;

void client::add_project(const std::filesystem::path& in_path) {
  auto k_conn = core_sql::Get().get_connection(in_path);
  k_conn->execute(R"(
create table if not exists metadatatab
(
    id          integer primary key,
    parent      integer            null,
    uuidPath    text               null,
    user_data   text               null,
    update_time datetime default CURRENT_TIMESTAMP not null,
    meta_type   bigint   default 0 null,
    episode     int                null,
    shot        int                null,
    season      int                null,
    assets_p    text               null,
    uuid_data   text               null,
    constraint fk_test_id
        foreign key (parent) references metadatatab (id)
            on delete cascade
);
)");

  k_conn->execute(R"(
create index IF NOT EXISTS Metadata_parent_index
    on metadatatab (parent);
)");
  k_conn->execute(R"(
create index IF NOT EXISTS Metadata_episode_index
    on metadatatab (episode);
)");
  k_conn->execute(R"(
create index IF NOT EXISTS Metadata_shot_index
    on metadatatab (shot);
)");
  k_conn->execute(R"(
create index IF NOT EXISTS Metadata_uuidPath_index
    on metadatatab (uuidPath);
)");
  k_conn->execute(R"(
create table if not exists usertab
(
    id               integer primary key,
    user_name        text not null,
    uuid_path        text null,
    user_data        text null,
    permission_group bigint default 0 not null
);

)");
  k_conn->execute(R"(
create index IF NOT EXISTS usertab_uuid_path_index
    on usertab (uuid_path);
)");
  k_conn->execute(R"(
create index IF NOT EXISTS usertab_user_name_index
    on usertab (user_name);
)");
  k_conn->execute(R"(
create table if not exists doodle_info
(
    version_major integer not null,
    version_minor integer not null
);
)");

  /// @brief 创建触发器
  k_conn->execute(R"(
create trigger UpdataLastTime AFTER UPDATE OF user_data,uuidPath,parent
    ON metadatatab
begin
    update metadatatab set update_time =CURRENT_TIMESTAMP where id = old.id;
end;
)");
  doodle_info::doodle_info l_info{};
  if (((*k_conn)(sqlpp::select(all_of(l_info)).from(l_info).unconditionally())).empty())
    (*k_conn)(sqlpp::insert_into(l_info)
                  .set(l_info.version_major = version::version_major,
                       l_info.version_minor = version::version_minor));
}
void client::open_project(const FSys::path& in_path) {
  g_reg()->ctx<core_sig>().project_begin_open(in_path);
  p_i->data_path = in_path;
  p_i->up_data();

  g_main_loop()
      .attach<database_task_select>(in_path)
      .then<one_process_t>([=]() {
        auto& k_reg = *g_reg();
        auto k_prj  = k_reg.view<project>();
        for (auto&& [e, p] : k_prj.each()) {
          /// @brief 这里我们强制将项目路径更改为项目所在路径
          p.p_path = in_path.parent_path();
          k_reg.ctx<core_sig>().project_end_open(make_handle(e), p);
          return;
        }
        chick_true<doodle_error>(false, DOODLE_LOC, "在这个库中找不到项目");
      });
}
void client::new_project(const entt::handle& in_handle) {
  chick_true<doodle_error>(in_handle.all_of<project>(), DOODLE_LOC, "缺失组件");
  auto k_prj     = in_handle.get<project>();

  auto k_path    = k_prj.get_path() / (k_prj.p_en_str + doodle_config::doodle_db_name.data());
  p_i->data_path = k_path;
  if (!in_handle.all_of<database>())
    in_handle.emplace<database>();
  add_project(k_path);
  g_reg()->set<project>(in_handle.get<project>());
  g_reg()->ctx<database_info>().path_ = k_path;
  g_main_loop()
      .attach<database_task_install>(in_handle)
      .then<one_process_t>([k_path, this]() {
        client{}.open_project(k_path);
      });
}

}  // namespace doodle::core
