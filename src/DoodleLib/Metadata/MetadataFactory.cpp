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
std::vector<ProjectPtr> MetadataFactory::getAllProject() const {
  return p_rpcClien->GetProject();
}

bool MetadataFactory::insert_into(Metadata *in_metadata) const {
  p_rpcClien->InstallMetadata(in_metadata->shared_from_this());
  return true;
}
void MetadataFactory::updata_db(MetadataPtr &in_metadata) const {
  this->p_rpcClien->UpdataMetadata(in_metadata, in_metadata->p_updata_parent_id);
}

void MetadataFactory::select_indb(MetadataPtr &in_metadata) const {
  if (!in_metadata->hasChild())
    return;
  auto k_c = this->p_rpcClien->GetChild(in_metadata);
  in_metadata->setChildItems(k_c);
}

void MetadataFactory::deleteData(const Metadata *in_metadata) const {
  p_rpcClien->DeleteMetadata(in_metadata->shared_from_this());
}

bool MetadataFactory::hasChild(const Metadata *in_metadata) const {
  // auto path = getRoot(in_metadata);
  // if (FSys::exists(path))
  //   return FSys::directory_iterator{path} != FSys::directory_iterator{};
  // else
  return false;
}

void MetadataFactory::select_indb(Project *in_) const {
  this->p_rpcClien->GetMetadata(in_->shared_from_this());
  select_indb(in_->shared_from_this());
}
void MetadataFactory::select_indb(Shot *in_) const {
  select_indb(in_->shared_from_this());
}
void MetadataFactory::select_indb(Episodes *in_) const {
  select_indb(in_->shared_from_this());
}
void MetadataFactory::select_indb(Assets *in_) const {
  select_indb(in_->shared_from_this());
}
void MetadataFactory::select_indb(AssetsFile *in_) const {
  select_indb(in_->shared_from_this());
}

void MetadataFactory::updata_db(Project *in_project) const {
  this->updata_db(in_project->shared_from_this());
}

void MetadataFactory::updata_db(Shot *in_shot) const {
  this->updata_db(in_shot->shared_from_this());
}
void MetadataFactory::updata_db(Episodes *in_episodes) const {
  this->updata_db(in_episodes->shared_from_this());
}
void MetadataFactory::updata_db(Assets *in_assets) const {
  this->updata_db(in_assets->shared_from_this());
}
void MetadataFactory::updata_db(AssetsFile *in_assetsFile) const {
  this->updata_db(in_assetsFile->shared_from_this());
}

// void MetadataFactory::modifyParent(Metadata *in_metadata, const Metadata *in_old_parent) const {
//   auto k_old_path = getRoot(in_old_parent);
//   auto k_new_path = getRoot(in_metadata->getParent().get());
//   if (FSys::exists(k_old_path) && !FSys::exists(k_new_path)) {
//     if (!FSys::exists(k_new_path.parent_path()))
//       FSys::create_directory(k_new_path.parent_path());
//     FSys::rename(k_old_path, k_new_path);
//   } else {
//     throw DoodleError{"没有旧纪录或者新记录已存在"};
//   }
// }

void MetadataFactory::deleteData(const Project *in_metadata) const {
  deleteData(dynamic_cast<const Metadata *>(in_metadata));
}
void MetadataFactory::deleteData(const Shot *in_metadata) const {
  deleteData(dynamic_cast<const Metadata *>(in_metadata));
}
void MetadataFactory::deleteData(const Episodes *in_metadata) const {
  deleteData(dynamic_cast<const Metadata *>(in_metadata));
}
void MetadataFactory::deleteData(const Assets *in_metadata) const {
  deleteData(dynamic_cast<const Metadata *>(in_metadata));
}
void MetadataFactory::deleteData(const AssetsFile *in_metadata) const {
  deleteData(dynamic_cast<const Metadata *>(in_metadata));
}

bool MetadataFactory::insert_into(Project *in_metadata) const {
  insert_into(dynamic_cast<Metadata *>(in_metadata));
  return true;
}
bool MetadataFactory::insert_into(Shot *in_metadata) const {
  insert_into(dynamic_cast<Metadata *>(in_metadata));
  return true;
}
bool MetadataFactory::insert_into(Episodes *in_metadata) const {
  insert_into(dynamic_cast<Metadata *>(in_metadata));
  return true;
}
bool MetadataFactory::insert_into(Assets *in_metadata) const {
  insert_into(dynamic_cast<Metadata *>(in_metadata));
  return true;
}
bool MetadataFactory::insert_into(AssetsFile *in_metadata) const {
  insert_into(dynamic_cast<Metadata *>(in_metadata));
  return true;
}

}  // namespace doodle
