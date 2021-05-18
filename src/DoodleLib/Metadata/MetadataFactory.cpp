//
// Created by TD on 2021/5/7.
//

#include <DoodleLib/Metadata/MetadataFactory.h>

#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/Shot.h>
#include <DoodleLib/Metadata/AssetsFile.h>
#include <DoodleLib/Metadata/Assets.h>
#include <DoodleLib/Metadata/AssetsPath.h>
#include <DoodleLib/Metadata/Comment.h>

#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/Logger/Logger.h>

#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/json.hpp>

namespace doodle {

MetadataFactory::MetadataFactory() {
}
FSys::path MetadataFactory::getRoot(const Metadata *in_metadata) const {
  auto k_prj    = coreSet::getSet().GetMetadataSet().Project_();
  auto k_config = k_prj->getPath() / Project::getConfigFileFolder() / in_metadata->getRoot();
  return k_config;
}
void MetadataFactory::loadChild(Metadata *in_metadata, const FSys::path &k_config) const {
  FSys::fstream k_fstream{};
  if (FSys::exists(k_config)) {
    in_metadata->clearChildItems();
    for (const auto &it : FSys::directory_iterator{k_config}) {
      k_fstream.open(it, std::ios::in | std::ios::binary);
      {
        std::shared_ptr<Metadata> k_ptr;
        cereal::PortableBinaryInputArchive k_archive{k_fstream};
        k_archive(k_ptr);
        if (k_ptr->checkParent(*in_metadata))
          k_ptr->setParent(in_metadata->shared_from_this());
        else
          DOODLE_LOG_INFO("父子uuid核实出错" << k_ptr->showStr());
      }
      k_fstream.close();
    }
    // in_metadata->sortChildItems();
  }
}
void MetadataFactory::load(Project *in_project) const {
  auto k_config_folder = in_project->getPath() / Project::getConfigFileFolder();
  auto k_path          = k_config_folder / Project::getConfigFileName();

  if (!FSys::exists(k_path))
    throw DoodleError{"Project non-existent"};

  Project k_p;
  FSys::fstream k_fstream{k_path, std::ios::in | std::ios::binary};
  {
    cereal::PortableBinaryInputArchive k_archive{k_fstream};
    k_archive(k_p);
  }
  k_fstream.close();

  if (k_p.getPath() == in_project->getPath())
    *in_project = k_p;
  else
    throw DoodleError{"Project inconsistency"};

  auto k_config = getRoot(in_project);
  loadChild(in_project, k_config);
}
void MetadataFactory::load(Shot *in_shot) const {
  auto k_config = getRoot(in_shot);
  loadChild(in_shot, k_config);
}
void MetadataFactory::load(Episodes *in_episodes) const {
  auto k_config = getRoot(in_episodes);
  loadChild(in_episodes, k_config);
}
void MetadataFactory::load(Assets *in_assets) const {
  auto k_config = getRoot(in_assets);
  loadChild(in_assets, k_config);
}
void MetadataFactory::load(AssetsFile *in_assetsFile) const {
  auto k_config = getRoot(in_assetsFile);
  loadChild(in_assetsFile, k_config);
}

void MetadataFactory::save(const Project *in_project) const {
  auto k_config_folder = in_project->getPath() / Project::getConfigFileFolder();
  auto k_path          = k_config_folder / Project::getConfigFileName();

  if (!FSys::exists(k_path.parent_path()))
    FSys::create_directories(k_path.parent_path());
  coreSet::hideFolder(k_path.parent_path());

  FSys::fstream k_fstream{k_path, std::ios::out | std::ios::binary};

  cereal::PortableBinaryOutputArchive k_archive{k_fstream};
  k_archive(*in_project);

  auto k_Floder = k_config_folder / in_project->getRoot();
  if (!FSys::exists(k_Floder))
    FSys::create_directories(k_Floder);
}

void MetadataFactory::save(const Shot *in_shot) const {
  if (!in_shot->hasParent())
    throw DoodleError{"not find Project"};

  auto k_ptr  = in_shot->getParent();
  auto k_path = this->getRoot(k_ptr.get()) / in_shot->getName();
  save(in_shot, k_path);
}
void MetadataFactory::save(const Episodes *in_episodes) const {
  if (!in_episodes->hasParent())
    throw DoodleError{"not find Project"};

  auto k_ptr  = in_episodes->getParent();
  auto k_path = this->getRoot(k_ptr.get()) / in_episodes->getName();
  save(in_episodes, k_path);
}
void MetadataFactory::save(const Assets *in_assets) const {
  if (!in_assets->hasParent())
    throw DoodleError{"not find Project"};

  auto k_ptr  = in_assets->getParent();
  auto k_path = this->getRoot(k_ptr.get()) / in_assets->getName();
  save(in_assets, k_path);
}
void MetadataFactory::save(const AssetsFile *in_assetsFile) const {
  if (!in_assetsFile->hasParent())
    throw DoodleError{"not find Project"};

  auto k_ptr  = in_assetsFile->getParent();
  auto k_path = this->getRoot(k_ptr.get()) / in_assetsFile->getName();
  save(in_assetsFile, k_path);
}
void MetadataFactory::save(const Metadata *in_metadata, const FSys::path &in_path) const {
  auto p_p = in_path.parent_path();
  if (!FSys::exists(p_p))
    FSys::create_directory(p_p);
  FSys::fstream file{in_path, std::ios::out | std::ios::binary};
  cereal::PortableBinaryOutputArchive k_archive{file};
  k_archive(in_metadata->shared_from_this());
}

void MetadataFactory::modifyParent(const Project *in_project, const Metadata *in_old_parent) const {
}

void MetadataFactory::modifyParent(const Shot *in_shot, const Metadata *in_old_parent) const {
  modifyParent(dynamic_cast<const Metadata*>(in_shot), in_old_parent);
  save(in_shot);
}

void MetadataFactory::modifyParent(const Episodes *in_episodes, const Metadata *in_old_parent) const {
  modifyParent(dynamic_cast<const Metadata*>(in_episodes), in_old_parent);
  save(in_episodes);
}

void MetadataFactory::modifyParent(const Assets *in_assets, const Metadata *in_old_parent) const {
  modifyParent(dynamic_cast<const Metadata*>(in_assets), in_old_parent);
  save(in_assets);
}

void MetadataFactory::modifyParent(const AssetsFile *in_assetsFile, const Metadata *in_old_parent) const {
  modifyParent(dynamic_cast<const Metadata*>(in_assetsFile), in_old_parent);
  save(in_assetsFile);
}

void MetadataFactory::modifyParent(const Metadata *in_metadata, const Metadata *in_old_parent) const {
  auto k_old_path = getRoot(in_old_parent) / in_metadata->getName();
  auto k_new_path = getRoot(in_metadata->getParent().get()) / in_metadata->getName();
  if (FSys::exists(k_old_path) && !FSys::exists(k_new_path)) {
    if (!FSys::exists(k_new_path.parent_path()))
      FSys::create_directory(k_new_path.parent_path());
    FSys::rename(k_old_path, k_new_path);
  } else {
    throw DoodleError{"没有旧纪录或者新记录已存在"};
  }
}
}  // namespace doodle
