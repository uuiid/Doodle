//
// Created by TD on 2021/5/7.
//
#include "metadata_factory.h"

#include <core/doodle_lib.h>
#include <doodle_lib/exception/exception.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/metadata/metadata_cpp.h>
#include <doodle_lib/thread_pool/long_term.h>

#include <long_task/database_task.h>

#ifdef DOODLE_GRPC
#include <doodle_lib/rpc/rpc_metadata_client.h>
#include <grpcpp/grpcpp.h>
#endif

#include <boost/fusion/algorithm/iteration/for_each.hpp>
namespace doodle {

class metadata_serialize::impl {
 public:
#ifdef DOODLE_GRPC
  std::weak_ptr<rpc_metadata_client> p_rpcClien;
#endif
};

metadata_serialize::metadata_serialize()
    : p_i(std::make_unique<impl>()) {
}
std::vector<entt::entity> metadata_serialize::get_all_prj() const {
  auto k_h = make_handle();
  k_h.emplace<process_message>();
  g_main_loop().attach<database_task_select>(k_h,)




  return std::vector<entt::entity>();
}
bool metadata_serialize::insert_into(entt::entity in) const {
  return false;
}
void metadata_serialize::delete_data(entt::entity in) const {
}
void metadata_serialize::updata_db(entt::entity in) const {
}
void metadata_serialize::select_indb(entt::entity in) const {
}
void metadata_serialize::select_indb_by_root(entt::entity in_root) const {
}
metadata_serialize::~metadata_serialize() = default;

#ifdef DOODLE_GRPC
std::vector<entt::entity> metadata_serialize::get_all_prj() const {
  auto k_c      = this->p_rpcClien.lock();

  auto k_filter = new_object<rpc_filter::filter>();
  k_filter->set_meta_type(metadata_type::project_root);
  auto k_list = k_c->select_entity(k_filter);
  boost::for_each(k_list, [](const entt::entity &in) {
    auto k_h = make_handle(in);
    k_h.emplace_or_replace<root_ref>(k_h);
  });
  return k_list;
}

bool metadata_serialize::insert_into(entt::entity in) const {
  auto k_h = make_handle(in);
  if (!k_h)
    return false;
  chick_true<doodle_error>(k_h.all_of<root_ref, database>(), DOODLE_LOC, "缺失组件");

  auto &k_data = k_h.get<database>();
  if (!k_data.has_components()) {
    DOODLE_LOG_WARN("没有可供序列化的组件")
    return false;
  }
  auto k_p_h = k_h.get<root_ref>().root_handle();
  auto k_c   = this->p_rpcClien.lock();
  if (!k_data.is_install()) {
    if (k_p_h && k_p_h != k_h) {
      if (!k_p_h.get<database>().is_install()) {
        insert_into(k_p_h);
        k_data.p_parent_id = k_p_h.get<database>().get_id();
      }
      if (!(k_data.p_parent_id) || k_data.p_parent_id == 0) {
        k_data.p_parent_id = k_p_h.get<database>().get_id();
      }
    }
    k_c->install_metadata(k_data);
  } else
    k_c->update_metadata(k_data);

  k_h.get<database_stauts>().set<is_load>();
  return true;
}

void metadata_serialize::delete_data(entt::entity in) const {
  auto k_h = make_handle(in);
  if (!k_h)
    return;

  chick_true<doodle_error>(k_h.all_of<database>(), DOODLE_LOC, "缺失组件");
  auto &k_data = k_h.get<database>();

  auto k_c     = this->p_rpcClien.lock();
  k_c->delete_metadata(k_data);
  k_h.get<database_stauts>().set<is_load>();
  k_h.destroy();
}

void metadata_serialize::updata_db(entt::entity in) const {
  auto k_h = make_handle(in);
  if (!k_h)
    return;

  chick_true<doodle_error>(k_h.all_of<database>(), DOODLE_LOC, "缺失组件");

  auto &k_data = k_h.get<database>();
  if (k_data.has_components()) {
    auto k_c = this->p_rpcClien.lock();
    k_c->update_metadata(k_data);
    k_h.get<database_stauts>().set<is_load>();
  } else {
    delete_data(in);
  }
}

void metadata_serialize::select_indb(entt::entity in) const {
  auto k_h = make_handle(in);
  if (!k_h)
    return;

  chick_true<doodle_error>(k_h.all_of<database, root_ref>(), DOODLE_LOC, "缺失组件");
  auto k_c      = this->p_rpcClien.lock();
  auto &k_tree  = k_h.get<root_ref>();

  auto &k_data  = k_h.get<database>();
  auto k_filter = new_object<rpc_filter::filter>();
  /// 选择自身本身并更新
  k_filter->set_id(k_data.get_id());
  k_data = k_c->select_metadata(k_filter).front();
  // k_filter->reset();

  /// 选择子物体并更新
  // k_filter->set_parent_id(k_data.get_id());
  // auto k_v = k_c->select_entity(k_filter);
  // for (auto &i : k_v) {
  //   make_handle(i).get_or_emplace<root_ref>().set_root(k_tree.p_root);
  // }

  k_h.get<database_stauts>().set<is_load>();
}
void metadata_serialize::select_indb_by_root(entt::entity in_root) const {
  auto k_h = make_handle(in_root);

  chick_true<doodle_error>(k_h.all_of<database_root, database>(), DOODLE_LOC, "缺失组件");
  auto &k_data  = k_h.get<database>();

  auto k_filter = new_object<rpc_filter::filter>();
  k_filter->set_parent_id(k_data.get_id());
  k_filter->set_beg_off_is(k_h.get<database_root>().get_current_id());

  auto k_c = this->p_rpcClien.lock();
  auto k_v = k_c->select_entity(k_filter);
  k_h.patch<database_root>([&](database_root &in) {
    in.p_end = k_v.empty();
    if (!in.p_end) {
      auto &k_data    = make_handle(k_v.back()).get<database>();
      in.p_current_id = k_data.get_id();
      in.p_cout_rows += k_v.size();
    }
  });
  k_h.get<database_root>().p_end = k_v.empty();
  for (auto &i : k_v) {
    auto k_i_h = make_handle(i);
    k_i_h.get_or_emplace<root_ref>().set_root(in_root);
    k_i_h.get_or_emplace<database_stauts>().set<is_load>();
  }
}
#endif

}  // namespace doodle
