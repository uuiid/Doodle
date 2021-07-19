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
    k_i->p_metadata_flctory_ptr_ = this->shared_from_this();
  }
  return k_v;
}

bool MetadataFactory::insert_into(Metadata *in_metadata) const {
  p_rpcClien.lock()->InstallMetadata(in_metadata->shared_from_this());
  return true;
}
void MetadataFactory::updata_db(MetadataPtr &in_metadata) const {
  this->p_rpcClien.lock()->UpdateMetadata(in_metadata, in_metadata->p_updata_parent_id);
}

void MetadataFactory::select_indb(MetadataPtr &in_metadata) const {
  if (!in_metadata->hasChild())
    return;
  auto k_c                = this->p_rpcClien.lock()->GetChild(in_metadata);
  for (auto & k_i: k_c) {
    in_metadata->add_child(k_i);
  }
  in_metadata->child_item = k_c;
}

void MetadataFactory::deleteData(const Metadata *in_metadata) const {
  p_rpcClien.lock()->DeleteMetadata(in_metadata->shared_from_this());
}

bool MetadataFactory::hasChild(const Metadata *in_metadata) const {
  // auto path = getRoot(in_metadata);
  // if (FSys::exists(path))
  //   return FSys::directory_iterator{path} != FSys::directory_iterator{};
  // else
  return false;
}

void MetadataFactory::select_indb(Project *in_) const {
  auto k_ptr = in_->shared_from_this();
  this->p_rpcClien.lock()->GetMetadata(k_ptr);
  select_indb(k_ptr);
}
void MetadataFactory::select_indb(Shot *in_) const {
  auto k_ptr = in_->shared_from_this();
  select_indb(k_ptr);
}
void MetadataFactory::select_indb(Episodes *in_) const {
  auto k_ptr = in_->shared_from_this();
  select_indb(k_ptr);
}
void MetadataFactory::select_indb(Assets *in_) const {
  auto k_ptr = in_->shared_from_this();
  select_indb(k_ptr);
}
void MetadataFactory::select_indb(AssetsFile *in_) const {
  auto k_ptr = in_->shared_from_this();
  select_indb(k_ptr);
}

void MetadataFactory::updata_db(Project *in_project) const {
  auto k_ptr = in_project->shared_from_this();
  this->updata_db(k_ptr);
}

void MetadataFactory::updata_db(Shot *in_shot) const {
  auto k_ptr = in_shot->shared_from_this();
  this->updata_db(k_ptr);
}
void MetadataFactory::updata_db(Episodes *in_episodes) const {
  auto k_ptr = in_episodes->shared_from_this();
  this->updata_db(k_ptr);
}
void MetadataFactory::updata_db(Assets *in_assets) const {
  auto k_ptr = in_assets->shared_from_this();
  this->updata_db(k_ptr);
}
void MetadataFactory::updata_db(AssetsFile *in_assetsFile) const {
  auto k_ptr = in_assetsFile->shared_from_this();
  this->updata_db(k_ptr);
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
