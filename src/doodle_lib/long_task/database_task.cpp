//
// Created by TD on 2022/1/11.
//

#include "database_task.h"
#include <metadata/metadata_cpp.h>
#include <core/core_sql.h>
#include <thread_pool/thread_pool.h>
#include <thread_pool/long_term.h>
#include <generate/core/metadatatab_sql.h>

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
};
database_task::database_task(const entt::handle& in_handle, const database_n::filter& in_filter)
    : p_i(std::make_unique<impl>()) {
  chick_true<doodle_error>(in_handle.all_of<process_message>(), DOODLE_LOC, "缺失消息组件");
  p_i->filter_ = in_filter;
  p_i->handle_ = in_handle;
}
database_task::~database_task() = default;
void database_task::init() {
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
  for(const auto& row : (*k_conn)(l_select)){
    auto& l_data = p_i->list.emplace_back();
    l_data.user_data = ;
  }

}
void database_task::succeeded() {
}
void database_task::failed() {
}
void database_task::aborted() {
}
void database_task::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
}

}  // namespace doodle
