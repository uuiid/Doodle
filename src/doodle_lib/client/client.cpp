//
// Created by TD on 2022/1/13.
//

#include "client.h"

#include <doodle_lib/core/core_sql.h>
#include <doodle_lib/long_task/process_pool.h>
#include <doodle_lib/long_task/database_task.h>
#include <doodle_lib/thread_pool/long_term.h>
#include <doodle_lib/metadata/metadata.h>
#include <doodle_lib/core/core_sig.h>

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
void client::add_project(const std::filesystem::path& in_path) {
  auto k_conn = core_sql::Get().get_connection(in_path / doodle_config::doodle_db_name);
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
  doodle_info::doodle_info l_info{};
  if (((*k_conn)(sqlpp::select(all_of(l_info)).from(l_info).unconditionally())).empty())
    (*k_conn)(sqlpp::insert_into(l_info)
                  .set(l_info.version_major = Doodle_VERSION_MAJOR,
                       l_info.version_minor = Doodle_VERSION_MINOR));
}
void client::open_project(const FSys::path& in_path) {
  g_reg()->ctx<core_sig>().begin_open(in_path);
  auto k_h = make_handle();
  k_h.emplace<process_message>();
  g_main_loop()
      .attach<database_task_select>(k_h, in_path)
      .then<one_process_t>([=]() {
        auto& k_reg = *g_reg();
        auto k_prj  = k_reg.view<project>();
        for (auto&& [e, p] : k_prj.each()) {
          k_reg.ctx<core_sig>().end_open(make_handle(e), p);
          return;
        }
        chick_true<doodle_error>(false, DOODLE_LOC, "在这个库中找不到项目");
      });
}

}  // namespace doodle::core
