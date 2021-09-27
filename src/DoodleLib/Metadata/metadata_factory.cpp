//
// Created by TD on 2021/5/7.
//
#include "metadata_factory.h"

#include <DoodleLib/Exception/exception.h>
#include <DoodleLib/Logger/logger.h>
#include <DoodleLib/Metadata/metadata_cpp.h>
#include <DoodleLib/core/core_set.h>
#include <DoodleLib/rpc/RpcMetadataClient.h>
#include <core/doodle_lib.h>
#include <grpcpp/grpcpp.h>

namespace doodle {

metadata_factory::metadata_factory()
    : p_rpcClien(doodle_lib::Get().get_rpc_metadata_client()) {
}
std::vector<ProjectPtr> metadata_factory::getAllProject() {
  auto k_v = p_rpcClien.lock()->GetProject();
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
  k_c->InstallMetadata(in_metadata->shared_from_this());
  return true;
}
void metadata_factory::updata_db(metadata *in_metadata) const {
  ///在这里测试使用具有父级， 并且如果有父级， 还要更新父id， 那么就可以断定也要更新父级的记录
  auto k_c = this->p_rpcClien.lock();
  if (in_metadata->has_parent()) {
    auto k_p = in_metadata->p_parent.lock();
    k_p->updata_db();
  }
  k_c->UpdateMetadata(in_metadata->shared_from_this(), in_metadata->p_updata_parent_id);
}

void metadata_factory::select_indb(metadata *in_metadata) const {
  // if (!details::is_class<AssetsFile>(in_metadata) && !in_metadata->hasChild())
  //   return;
  auto k_c = this->p_rpcClien.lock()->GetChild(in_metadata->shared_from_this());
  for (auto &k_i : k_c) {
    in_metadata->add_child(k_i);
  }
  in_metadata->child_item = k_c;
}

void metadata_factory::deleteData(const metadata *in_metadata) const {
  p_rpcClien.lock()->DeleteMetadata(in_metadata->shared_from_this());
}

}  // namespace doodle
