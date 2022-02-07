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
      _off_size() {
}

}  // namespace database_n

class database_task_select::impl {
 public:
  database_n::filter filter_;
  std::vector<metadata_database> list;
  std::future<void> result;
  std::atomic_bool stop = false;
  FSys::path prj_root;
  std::size_t size{};
  std::vector<entt::handle> create_handle;
};
database_task_select::database_task_select(const database_n::filter& in_filter)
    : p_i(std::make_unique<impl>()) {
  p_i->filter_  = in_filter;
  p_i->prj_root = g_reg()->ctx<project>().get_path() / doodle_config::doodle_db_name;
}

database_task_select::database_task_select(const FSys::path& in_prj_root)
    : p_i(std::make_unique<impl>()) {
  //  p_i->filter_._meta_type = metadata_type::project_root;
  p_i->prj_root = in_prj_root / doodle_config::doodle_db_name;
}

database_task_select::~database_task_select() = default;

void database_task_select::select_db() {
  auto k_conn = core_sql::Get().get_connection_const(p_i->prj_root);
  Metadatatab l_metadatatab{};

  auto l_select = sqlpp::dynamic_select(*k_conn, sqlpp::all_of(l_metadatatab)).from(l_metadatatab).dynamic_where();
  if (p_i->filter_._begin && p_i->filter_._end) {
    l_select.where.add(l_metadatatab.updateTime > *p_i->filter_._begin &&
                       l_metadatatab.updateTime < *p_i->filter_._end);
  }
  if (p_i->filter_._meta_type)
    l_select.where.add(l_metadatatab.metaType == magic_enum::enum_integer(*p_i->filter_._meta_type));
  if (p_i->filter_._id)
    l_select.where.add(l_metadatatab.id == *p_i->filter_._id);
  if (p_i->filter_._parent_id)
    l_select.where.add(l_metadatatab.parent == *p_i->filter_._parent_id);

  if (p_i->filter_._off_size) {
    l_select.where.add(l_metadatatab.id > p_i->filter_._beg_off_id);
    l_select.limit(*p_i->filter_._off_size);
  }

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
      l_data.season = boost::numeric_cast<std::int32_t>(row.season.value());
    if (!row.episode.is_null())
      l_data.episode = boost::numeric_cast<std::int32_t>(row.episode.value());
    if (!row.shot.is_null())
      l_data.shot = boost::numeric_cast<std::int32_t>(row.shot.value());
    if (!row.parent.is_null())
      l_data.parent = boost::numeric_cast<decltype(l_data.parent)::value_type>(row.parent.value());
  }
  p_i->size = p_i->list.size();
  std::vector<entt::entity> l_vector{};
  l_vector.resize(p_i->list.size());
  g_reg()->create(l_vector.begin(), l_vector.end());
  std::transform(l_vector.begin(), l_vector.end(), std::back_inserter(p_i->create_handle), [](const auto& in) { return make_handle(in); });
}

void database_task_select::init() {
  auto& k_msg = g_reg()->set<process_message>();
  k_msg.set_name("加载数据");
  k_msg.set_state(k_msg.run);
  p_i->result = doodle_lib::Get().get_thread_pool()->enqueue([this]() { this->select_db(); });
}
void database_task_select::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
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
    //    make_handle().emplace<database>() = p_i->list.back();
    for (int l_i = 0; l_i < 3; ++l_i) {
      if (p_i->create_handle.empty())
        return;
      auto& k_data               = p_i->create_handle.back();
      k_data.emplace<database>() = p_i->list.back();
      k_data.patch<database>(database::sync{});

      g_reg()->ctx<process_message>().progress_step({1, p_i->size});

      p_i->create_handle.pop_back();
      p_i->list.pop_back();
    }
  }
}

void database_task_select::succeeded() {
  g_reg()->ctx<process_message>().set_state(process_message::success);
}
void database_task_select::failed() {
  g_reg()->ctx<process_message>().set_state(process_message::fail);
}
void database_task_select::aborted() {
  g_reg()->ctx<process_message>().set_state(process_message::fail);
  p_i->stop = true;
}

class database_task_update::impl {
 public:
  std::vector<entt::handle> list;

  FSys::path prj_root;
  std::atomic_bool stop;
  std::future<void> result;
};

database_task_update::database_task_update(const std::vector<entt::handle>& in_list)
    : p_i(std::make_unique<impl>()) {
  p_i->prj_root = g_reg()->ctx<project>().get_path() / doodle_config::doodle_db_name;
  p_i->list     = in_list;
}
database_task_update::~database_task_update() = default;
void database_task_update::init() {
  p_i->result = g_thread_pool().enqueue([this]() { this->update_db(); });
}
void database_task_update::update_db() {
  auto k_conn = core_sql::Get().get_connection(p_i->prj_root);
  Metadatatab l_metadatatab{};
  metadata_database k_data;
  for (auto& in : p_i->list) {
    if (p_i->stop)
      return;
    k_data     = in.get<database>();
    auto k_sql = sqlpp::dynamic_update(*k_conn, l_metadatatab).where(l_metadatatab.id == k_data.id).dynamic_set();
    k_sql.assignments.add(l_metadatatab.userData = k_data.user_data);
    k_sql.assignments.add(l_metadatatab.metaType = k_data.m_type);
    k_sql.assignments.add(l_metadatatab.uuidPath = k_data.uuid_path);

    if (k_data.parent)
      k_sql.assignments.add(l_metadatatab.parent = *k_data.parent);
    if (k_data.season)
      k_sql.assignments.add(l_metadatatab.season = *k_data.season);
    if (k_data.episode)
      k_sql.assignments.add(l_metadatatab.episode = *k_data.episode);
    if (k_data.shot)
      k_sql.assignments.add(l_metadatatab.shot = *k_data.shot);
    if (k_data.assets)
      k_sql.assignments.add(l_metadatatab.assetsP = *k_data.assets);
    (*k_conn)(k_sql);
  }
}
void database_task_update::succeeded() {
}
void database_task_update::failed() {
}
void database_task_update::aborted() {
  p_i->stop = true;
}
void database_task_update::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  switch (p_i->result.wait_for(0ns)) {
    case std::future_status::ready: {
      try {
        p_i->result.get();
        this->succeed();
      } catch (const doodle_error& error) {
        DOODLE_LOG_ERROR(error.what());
        this->fail();
        throw;
      }
    } break;
    default:
      break;
  }
}
database_task_update::database_task_update(const entt::handle& in_handle)
  : database_task_update(std::vector<entt::handle>{in_handle}){

}

class database_task_delete::impl {
 public:
  std::vector<entt::handle> list;
  std::future<void> result;
  std::atomic_bool stop = false;
  FSys::path prj_root;
};

void database_task_delete::delete_db() {
  auto k_conn = core_sql::Get().get_connection(p_i->prj_root);
  Metadatatab l_metadatatab{};
  for (auto& in : p_i->list) {
    if (p_i->stop)
      return;
    auto k_r = (*k_conn)(sqlpp::remove_from(l_metadatatab).where(l_metadatatab.id == in.get<database>().get_id()));
    DOODLE_LOG_WARN("删除了数据id {}", k_r);
  }
}
database_task_delete::database_task_delete(
    const std::vector<entt::handle>& in_list)
    : p_i(std::make_unique<impl>()) {
  p_i->prj_root = g_reg()->ctx<project>().get_path() / doodle_config::doodle_db_name;
  p_i->list     = in_list;
}
database_task_delete::~database_task_delete() = default;
void database_task_delete::init() {
  p_i->result = g_thread_pool().enqueue([this]() { this->delete_db(); });
}
void database_task_delete::succeeded() {
}
void database_task_delete::failed() {
}
void database_task_delete::aborted() {
  p_i->stop = true;
}
void database_task_delete::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  switch (p_i->result.wait_for(0ns)) {
    case std::future_status::ready: {
      try {
        p_i->result.get();
        this->succeed();
      } catch (const doodle_error& error) {
        DOODLE_LOG_ERROR(error.what());
        this->fail();
        throw;
      }
    } break;
    default:
      break;
  }
}
database_task_delete::database_task_delete(const entt::handle& in_handle)
    : database_task_delete(std::vector<entt::handle>{in_handle}) {
}

class database_task_install::impl {
 public:
  entt::handle handle_;
  std::vector<entt::handle> list;
  std::future<void> result;
  std::atomic_bool stop = false;
  FSys::path prj_root;
};

void database_task_install::install_db() {
  auto k_conn = core_sql::Get().get_connection(p_i->prj_root);
  Metadatatab l_metadatatab{};
  metadata_database k_data;
  for (auto& in : p_i->list) {
    if (p_i->stop)
      return;
    k_data     = in.get<database>();
    auto k_sql = sqlpp::dynamic_insert_into(*k_conn, l_metadatatab).dynamic_set();
    k_sql.insert_list.add(l_metadatatab.metaType = k_data.m_type);
    k_sql.insert_list.add(l_metadatatab.uuidPath = k_data.uuid_path);
    k_sql.insert_list.add(l_metadatatab.userData = k_data.user_data);

    if (k_data.parent)
      k_sql.insert_list.add(l_metadatatab.parent = *k_data.parent);
    if (k_data.season)
      k_sql.insert_list.add(l_metadatatab.season = *k_data.season);
    if (k_data.episode)
      k_sql.insert_list.add(l_metadatatab.episode = *k_data.episode);
    if (k_data.shot)
      k_sql.insert_list.add(l_metadatatab.shot = *k_data.shot);
    if (k_data.assets)
      k_sql.insert_list.add(l_metadatatab.assetsP = *k_data.assets);
    auto id = (*k_conn)(k_sql);
    in.patch<database>([&](database& in) {
      in.set_id(id);
    });
    in.patch<database>(database::sync{});
  }
}
database_task_install::database_task_install(
    const std::vector<entt::handle>& in_list)
    : p_i(std::make_unique<impl>()) {
  p_i->list = in_list;
}

database_task_install::~database_task_install() = default;

void database_task_install::init() {
  p_i->prj_root = g_reg()->ctx<project>().get_path() / doodle_config::doodle_db_name;

  p_i->result   = g_thread_pool().enqueue([this]() { this->install_db(); });
}
void database_task_install::succeeded() {
}
void database_task_install::failed() {
}
void database_task_install::aborted() {
}
void database_task_install::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  switch (p_i->result.wait_for(0ns)) {
    case std::future_status::ready: {
      try {
        p_i->result.get();
        this->succeed();
      } catch (const doodle_error& error) {
        DOODLE_LOG_ERROR(error.what());
        this->fail();
        throw;
      }
    } break;
    default:
      break;
  }
}
database_task_install::database_task_install(const entt::handle& in_handle)
    : database_task_install(std::vector<entt::handle>{in_handle}) {
}
class database_task_obs::impl {
 public:
  entt::observer obs;
  entt::handle handle_;
};
database_task_obs::database_task_obs()
    : p_i(std::make_unique<impl>()) {
}
database_task_obs::~database_task_obs() = default;

void database_task_obs::init() {
  p_i->obs.connect(*g_reg(), entt::collector.update<database>());
  g_reg()->set<database_task_obs&>(*this);
}
void database_task_obs::succeeded() {
}
void database_task_obs::failed() {
  p_i->obs.disconnect();
}
void database_task_obs::aborted() {
  p_i->obs.disconnect();
}
void database_task_obs::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  if (p_i->obs.empty())
    return;
  std::vector<entt::handle> k_handle_list{};
  auto& k_obs = p_i->obs;
  std::transform(k_obs.begin(), k_obs.end(), std::back_inserter(k_handle_list), [](auto& in_item) { return make_handle(in_item); });
  {
    /// \brief 必须首先保证没有需要插入的, 然后才开始更新和插入

    std::vector<entt::handle> k_h{};
    std::copy_if(k_handle_list.begin(), k_handle_list.end(), std::back_inserter(k_h),
                 [](const entt::handle& in_handle) {
                   auto&& k_data = in_handle.get<database>();
                   return k_data.status_ == database::status::need_save && !k_data.is_install();
                 });
    if (!k_h.empty())
      g_main_loop().attach<database_task_install>(k_h);
  }
  {
    std::vector<entt::handle> k_h{};
    std::copy_if(k_handle_list.begin(), k_handle_list.end(), std::back_inserter(k_h),
                 [](const entt::handle& in_handle) {
                   auto&& k_data = in_handle.get<database>();
                   return k_data.status_ == database::status::need_save && k_data.is_install();
                 });
    if (!k_h.empty())
      g_main_loop().attach<database_task_update>(k_h);
  }
  {
    std::vector<entt::handle> k_h{};
    std::copy_if(k_handle_list.begin(), k_handle_list.end(), std::back_inserter(k_h),
                 [](const entt::handle& in_handle) {
                   return in_handle.get<database>().status_ == database::status::need_delete;
                 });
    if (!k_h.empty())
      g_main_loop().attach<database_task_delete>(k_h);
  }
  k_obs.clear();
}
}  // namespace doodle
