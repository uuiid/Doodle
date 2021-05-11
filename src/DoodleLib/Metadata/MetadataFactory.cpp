//
// Created by TD on 2021/5/7.
//

#include <DoodleLib/Metadata/MetadataFactory.h>

#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/Shot.h>
#include <DoodleLib/Metadata/AssetsFile.h>
#include <DoodleLib/Metadata/Assets.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/Exception/Exception.h>

#include <core/coreset.h>
#include <cereal/archives/portable_binary.hpp>

namespace doodle {

MetadataFactory::MetadataFactory() {
}
FSys::path MetadataFactory::GetRoot(const Metadata *in_metadata) const {
  auto k_prj = coreSet::getSet().GetMetadataSet().Project_();
  auto k_config = k_prj->Path() / Project::getConfigFileFolder() / in_metadata->GetRoot();
  return k_config;
}
void MetadataFactory::loadChild(Metadata *in_metadata, const FSys::path &k_config) const {
  FSys::fstream k_fstream{};
  if (FSys::exists(k_config))
    for (const auto &it : FSys::directory_iterator{k_config}) {
      k_fstream.open(it, std::ios::in | std::ios::binary);
      {
        std::shared_ptr<Metadata> k_ptr;
        cereal::PortableBinaryInputArchive k_archive{k_fstream};
        k_archive(k_ptr);
        if (in_metadata->checkParent(*in_metadata))
          in_metadata->AddChildItem(k_ptr);
      }
      k_fstream.close();
    }
}
void MetadataFactory::load(Project *in_project) const {
  auto k_config_folder = in_project->Path() / Project::getConfigFileFolder();
  auto k_path = k_config_folder / Project::getConfigFileName();

  if (!FSys::exists(k_path))
    throw DoodleError{"Project non-existent"};

  Project k_p;
  FSys::fstream k_fstream{k_path, std::ios::in | std::ios::binary};
  {
    cereal::PortableBinaryInputArchive k_archive{k_fstream};
    k_archive(k_p);
  }
  k_fstream.close();

  if (k_p.Path() == in_project->Path())
    *in_project = k_p;
  else
    throw DoodleError{"Project inconsistency"};

  auto k_config = GetRoot(in_project);
  loadChild(in_project, k_config);
}
void MetadataFactory::load(Shot *in_shot) const {
  auto k_config = GetRoot(in_shot);
  loadChild(in_shot, k_config);
}
void MetadataFactory::load(Episodes *in_episodes) const {
  auto k_config = GetRoot(in_episodes);
  loadChild(in_episodes, k_config);
}
void MetadataFactory::load(Assets *in_assets) const {
  auto k_config = GetRoot(in_assets);
  loadChild(in_assets, k_config);
}
void MetadataFactory::load(AssetsFile *in_assetsFile) const {
  auto k_config = GetRoot(in_assetsFile);
  loadChild(in_assetsFile, k_config);
}

void MetadataFactory::save(const Project *in_project) const {
  auto k_config_folder = in_project->Path() / Project::getConfigFileFolder();
  auto k_path = k_config_folder / Project::getConfigFileName();

  if (FSys::exists(k_path.parent_path()))
    FSys::create_directories(k_path.parent_path());

  FSys::fstream k_fstream{k_path, std::ios::out | std::ios::binary};

  cereal::PortableBinaryOutputArchive k_archive{k_fstream};
  k_archive(*in_project);

  auto k_Floder = k_config_folder / in_project->GetRoot();
  if (FSys::exists(k_Floder))
    FSys::create_directories(k_Floder);
}

void MetadataFactory::save(const Shot *in_shot) const {
  if (!in_shot->HasParent())
    throw DoodleError{"not find Project"};

  auto k_ptr = in_shot->GetPParent();
  auto k_path = this->GetRoot(k_ptr.get()) / in_shot->GetName();
  save(in_shot, k_path);
}
void MetadataFactory::save(const Episodes *in_episodes) const {
  if (!in_episodes->HasParent())
    throw DoodleError{"not find Project"};

  auto k_ptr = in_episodes->GetPParent();
  auto k_path = this->GetRoot(k_ptr.get()) / in_episodes->GetName();
  save(in_episodes, k_path);
}
void MetadataFactory::save(const Assets *in_assets) const {
  if (!in_assets->HasParent())
    throw DoodleError{"not find Project"};

  auto k_ptr = in_assets->GetPParent();
  auto k_path = this->GetRoot(k_ptr.get()) / in_assets->GetName();
  save(in_assets, k_path);
}
void MetadataFactory::save(const AssetsFile *in_assetsFile) const {
  if (!in_assetsFile->HasParent())
    throw DoodleError{"not find Project"};

  auto k_ptr = in_assetsFile->GetPParent();
  auto k_path = this->GetRoot(k_ptr.get()) / in_assetsFile->GetName();
  save(in_assetsFile, k_path);
}
void MetadataFactory::save(const Metadata *in_metadata, const FSys::path &in_path) const {
  FSys::fstream file{in_path, std::ios::out | std::ios::binary};
  cereal::PortableBinaryOutputArchive k_archive{file};
  k_archive(*in_metadata);
}
}  // namespace doodle
