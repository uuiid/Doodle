//
// Created by TD on 2021/5/7.
//

#pragma once
#include <corelib/core_global.h>
namespace doodle {
class MetadataFactory {
 public:
  MetadataFactory();

  void load(Project* in_project) const;
  void load(Shot* in_shot) const;
  void load(Episodes* in_episodes) const;
  void load(Assets* in_assets) const;
  void load(AssetsFile* in_assetsFile) const;

  void save(const Project* in_project) const;
  void save(const Shot* in_shot) const;
  void save(const Episodes* in_episodes) const;
  void save(const Assets* in_assets) const;
  void save(const AssetsFile* in_assetsFile) const;

 private:
  void loadChild(Metadata* in_metadata, const FSys::path& k_config) const;
  FSys::path GetRoot(const Metadata* in_metadata) const;
  void save(const Metadata* in_metadata, const FSys::path& in_path) const;
};

}  // namespace doodle
