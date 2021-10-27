//
// Created by TD on 2021/5/7.
//
#include "metadata_factory.h"

#include <core/doodle_lib.h>
#include <doodle_lib/Exception/exception.h>
#include <doodle_lib/Logger/logger.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/metadata/metadata_cpp.h>
#include <doodle_lib/rpc/rpc_metadata_client.h>
#include <grpcpp/grpcpp.h>

namespace doodle {

metadata_factory::metadata_factory()
    : p_rpcClien(doodle_lib::Get().get_rpc_metadata_client()) {
}
std::vector<project_ptr> metadata_factory::getAllProject() {
  auto k_v = p_rpcClien.lock()->get_project();
  for (auto &k_i : k_v) {
    k_i->p_factory = this->shared_from_this();
  }
  return k_v;
}

bool metadata_factory::insert_into(metadata *in_metadata) const {
  auto k_c = this->p_rpcClien.lock();
  if (in_metadata->has_parent()) {
    auto k_p = in_metadata->p_parent.lock();
    k_p->updata_db();
  }
  k_c->install_metadata(in_metadata->shared_from_this());
  return true;
}
void metadata_factory::updata_db(metadata *in_metadata) const {
  ///在这里测试使用具有父级， 并且如果有父级， 还要更新父id， 那么就可以断定也要更新父级的记录
  auto k_c = this->p_rpcClien.lock();
  if (in_metadata->has_parent()) {
    auto k_p = in_metadata->p_parent.lock();
    k_p->updata_db();
  }
  k_c->update_metadata(in_metadata->shared_from_this(), in_metadata->p_updata_parent_id);
}

void metadata_factory::select_indb(metadata *in_metadata) const {
  // if (!details::is_class<AssetsFile>(in_metadata) && !in_metadata->hasChild())
  //   return;
  auto k_c = this->p_rpcClien.lock()->get_child(in_metadata->shared_from_this());
  for (auto &k_i : k_c) {
    in_metadata->add_child(k_i);
  }
  in_metadata->child_item = k_c;
}

void metadata_factory::delete_data(const metadata *in_metadata) const {
  auto k_meta = in_metadata->shared_from_this();
  p_rpcClien.lock()->delete_metadata(k_meta);
  auto k_p = k_meta->get_parent();
  k_p->updata_db();
}

metadata_serialize::metadata_serialize()
    : p_rpcClien(doodle_lib::Get().get_rpc_metadata_client()) {
}

std::vector<entt::entity> metadata_serialize::get_all_prj() const {
  auto k_c      = this->p_rpcClien.lock();

  auto k_filter = new_object<rpc_filter::filter>();
  k_filter->set_meta_type(metadata::meta_type::project_root);
  auto k_list = k_c->select_entity(k_filter);
  return k_list;
}

bool metadata_serialize::insert_into(entt::entity in) const {
  auto k_reg  = g_reg();
  auto k_tree = k_reg->try_get<tree_relationship>(in);

  auto k_data = k_reg->try_get<database>(in);
  if (k_data) {
    if (k_tree) {
      if (k_tree->has_parent()) {
        updata_db(k_tree->get_parent());
      }
    }
    auto k_c = this->p_rpcClien.lock();
    if (!k_data->is_install())
      k_c->install_metadata(*k_data);
    else
      k_c->update_metadata(*k_data);
  } else
    throw doodle_error{"没有数据库组件"};
}

void metadata_serialize::delete_data(entt::entity in) const {
  auto k_reg  = g_reg();
  auto k_tree = k_reg->try_get<tree_relationship>(in);

  auto k_data = k_reg->try_get<database>(in);
  if (k_data) {
    if (k_tree) {
      if (k_tree->has_parent()) {
        updata_db(k_tree->get_parent());
      }
    }
    auto k_c = this->p_rpcClien.lock();
    k_c->delete_metadata(*k_data);
  }
}

void metadata_serialize::updata_db(entt::entity in) const {
  auto k_reg  = g_reg();
  auto k_tree = k_reg->try_get<tree_relationship>(in);

  auto k_data = k_reg->try_get<database>(in);
  if (k_data) {
    if (k_data->p_need_save) {
      if (k_tree) {
        if (k_tree->has_parent()) {
          updata_db(k_tree->get_parent());
        }
      }
      auto k_c = this->p_rpcClien.lock();
      k_c->update_metadata(*k_data);
    }
  }
}

void metadata_serialize::select_indb(entt::entity in) const {
  auto k_c      = this->p_rpcClien.lock();
  auto k_reg    = g_reg();
  auto k_tree   = k_reg->try_get<tree_relationship>(in);

  auto k_data   = k_reg->try_get<database>(in);
  auto k_filter = new_object<rpc_filter::filter>();
  /// 选择自身本身并更新

  // k_filter->set_id(k_data->get_id());
  // *k_data = k_c->select_metadata(k_filter).front();
  // k_filter->reset();

  /// 选择子物体并更新
  k_filter->set_parent_id(k_data->get_id());
  auto k_v = k_c->select_entity(k_filter);
  for (auto &i : k_v) {
    auto &k = k_reg->get_or_emplace<tree_relationship>(i);
    k.set_parent(in);
  }
}

}  // namespace doodle
