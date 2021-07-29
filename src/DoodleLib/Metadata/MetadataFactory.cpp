//
// Created by TD on 2021/5/7.
//
#include "MetadataFactory.h"

#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/Metadata/Metadata_cpp.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/rpc/RpcMetadataClient.h>
#include <grpcpp/grpcpp.h>

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>

namespace doodle {

MetadataFactory::MetadataFactory()
    : p_rpcClien(CoreSet::getSet().getRpcMetadataClient()) {
}
std::vector<ProjectPtr> MetadataFactory::getAllProject() {
  auto k_v = p_rpcClien.lock()->GetProject();
  for (auto &k_i : k_v) {
    k_i->p_factory = this->shared_from_this();
  }
  return k_v;
}

bool MetadataFactory::insert_into(Metadata *in_metadata) const {
  p_rpcClien.lock()->InstallMetadata(in_metadata->shared_from_this());
  return true;
}
void MetadataFactory::updata_db(Metadata *in_metadata) const {
  this->p_rpcClien.lock()->UpdateMetadata(in_metadata->shared_from_this(), in_metadata->p_updata_parent_id);
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
