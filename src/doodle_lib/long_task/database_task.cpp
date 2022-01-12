//
// Created by TD on 2022/1/11.
//

#include "database_task.h"
#include <metadata/metadata_cpp.h>
#include <core/core_sql.h>
#include <thread_pool/thread_pool.h>
#include <thread_pool/long_term.h>
#include <generate/core/metadatatab_sql.h>
#include <core/doodle_lib.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>

namespace doodle {
namespace database_n {

filter::filter()
    : _id(),
      _parent_id(),
      _meta_type(),
      _begin(),
      _end(),

      _episodes(),
      _shot(),
      _assets(),
      _beg_off_id(0u),
      _off_size(1000u) {
}

}  // namespace database_n

class database_task::impl {
 public:
  database_n::filter filter_;
  entt::handle handle_;
  std::vector<metadata_database> list;
  std::future<void> result;
  std::atomic_bool stop = false;
};
database_task::database_task(const entt::handle& in_handle, const database_n::filter& in_filter)
    : p_i(std::make_unique<impl>()) {
  chick_true<doodle_error>(in_handle.all_of<process_message>(), DOODLE_LOC, "缺失消息组件");
  p_i->filter_ = in_filter;
  p_i->handle_ = in_handle;
}
database_task::~database_task() = default;
void database_task::select_db() {
  auto k_conn = core_sql::Get().get_connection(g_reg()->ctx<project>().get_path() / "doodle_db");
  Metadatatab l_metadatatab{};

  auto l_select = sqlpp::dynamic_select(*k_conn, sqlpp::all_of(l_metadatatab)).from(l_metadatatab).dynamic_where();
  if (p_i->filter_._begin && p_i->filter_._end) {
    l_select.where.add(l_metadatatab.updateTime > *p_i->filter_._begin &&
                       l_metadatatab.updateTime < *p_i->filter_._end);
  }
  if (p_i->filter_._meta_type)
    l_select.where.add(l_metadatatab.metaType == p_i->filter_._meta_type);
  if (p_i->filter_._id)
    l_select.where.add(l_metadatatab.id == p_i->filter_._id);
  if (p_i->filter_._parent_id)
    l_select.where.add(l_metadatatab.parent == p_i->filter_._parent_id);

  l_select.where.add(l_metadatatab.id > p_i->filter_._beg_off_id);
  l_select.limit(p_i->filter_._off_size);
  for (const auto& row : (*k_conn)(l_select)) {
    if (p_i->stop)
      return;
    auto& l_data     = p_i->list.emplace_back();
    l_data.user_data = row.userData.value();
    l_data.id        = row.id.value();
    l_data.uuid_path = row.uuidPath.value();
    l_data.m_type    = boost::numeric_cast<decltype(l_data.m_type)>(row.metaType.value());
    if (!row.assetsP.is_null())
      l_data.assets = row.assetsP.value();
    if (!row.season.is_null())
      l_data.season = row.season.value();
    if (!row.episode.is_null())
      l_data.episode = row.episode.value();
    if (!row.shot.is_null())
      l_data.shot = row.shot.value();
    if (!row.parent.is_null())
      l_data.parent = row.parent.value();
  }
}
void database_task::init() {
  p_i->handle_.patch<process_message>([](process_message& in) {
                in.set_name("加载文件");
                in.set_state(in.run);
              })
      .aborted_function = [self = this]() {
    if (self) {
      self->p_i->stop = true;
      self->abort();
    }
  };
  p_i->result = doodle_lib::Get().get_thread_pool()->enqueue([this]() { this->select_db(); });
}
void database_task::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  if (p_i->result.valid())
    switch (p_i->result.wait_for(0ns)) {
      case std::future_status::ready: {
        try {
          p_i->result.get();
        } catch (const doodle_error& error) {
          DOODLE_LOG_ERROR(error.what());
          this->fail();
          throw;
        }
      } break;
      default:
        break;
    }
  else {
    if (p_i->list.empty()) {
      this->succeed();
      return;
    }
    make_handle().emplace<database>(p_i->list.back());
    p_i->list.pop_back();
  }
}

void database_task::succeeded() {
}
void database_task::failed() {
}
void database_task::aborted() {
  DOODLE_LOG_ERROR("用户主动结束进程");
}

}  // namespace doodle
