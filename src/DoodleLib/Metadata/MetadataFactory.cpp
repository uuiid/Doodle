//
// Created by TD on 2021/5/7.
//
#include "MetadataFactory.h"

#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/Metadata/Metadata_cpp.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/rpc/RpcMetadataClient.h>
#include <core/DoodleLib.h>
#include <grpcpp/grpcpp.h>

namespace doodle {

MetadataFactory::MetadataFactory()
    : p_rpcClien(DoodleLib::Get().getRpcMetadataClient()) {
}
std::vector<ProjectPtr> MetadataFactory::getAllProject() {
  auto k_v = p_rpcClien.lock()->GetProject();
  for (auto &k_i : k_v) {
    k_i->p_factory = this->shared_from_this();
  }
  return k_v;
}

bool MetadataFactory::insert_into(Metadata *in_metadata) const {
  auto k_c = this->p_rpcClien.lock();
  if (in_metadata->hasParent()) {
    auto k_p = in_metadata->p_parent.lock();
    k_p->updata_db();
  }
  k_c->InstallMetadata(in_metadata->shared_from_this());
  return true;
}
void MetadataFactory::updata_db(Metadata *in_metadata) const {
  ///在这里测试使用具有父级， 并且如果有父级， 还要更新父id， 那么就可以断定也要更新父级的记录
  auto k_c = this->p_rpcClien.lock();
  if (in_metadata->hasParent()) {
    auto k_p = in_metadata->p_parent.lock();
    k_p->updata_db();
  }
  k_c->UpdateMetadata(in_metadata->shared_from_this(), in_metadata->p_updata_parent_id);
}

void MetadataFactory::select_indb(Metadata *in_metadata) const {
  if (!in_metadata->hasChild())
    return;
  auto k_c = this->p_rpcClien.lock()->GetChild(in_metadata->shared_from_this());
  for (auto &k_i : k_c) {
    in_metadata->add_child(k_i);
  }
  in_metadata->child_item = k_c;
}

void MetadataFactory::deleteData(const Metadata *in_metadata) const {
  p_rpcClien.lock()->DeleteMetadata(in_metadata->shared_from_this());
}

}  // namespace doodle
