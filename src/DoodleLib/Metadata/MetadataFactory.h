//
// Created by TD on 2021/5/7.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
namespace doodle {
/**
  * 父亲负责加载孩子，在加载父亲时，孩子会同时加载完成， 这样， 父亲会知道是否加载玩成孩子
  * 在保存时，保存路径是： prjRoot /父亲的root（uuid）/孩子的（root）uuid（文件）
  * 这样，我们就形成了逻辑闭环
  * 同时， 我们的父亲加载孩子时还会验证孩子的父子是否一致
  *
  * @warning 我们在更改父亲时， 要同时移动文件和保存更改
  */
class DOODLELIB_API MetadataFactory {
 public:
  MetadataFactory();

  virtual void load(Project* in_project) const;
  virtual void load(Shot* in_shot) const;
  virtual void load(Episodes* in_episodes) const;
  virtual void load(Assets* in_assets) const;
  virtual void load(AssetsFile* in_assetsFile) const;

  virtual void save(const Project* in_project) const;
  virtual void save(const Shot* in_shot) const;
  virtual void save(const Episodes* in_episodes) const;
  virtual void save(const Assets* in_assets) const;
  virtual void save(const AssetsFile* in_assetsFile) const;

  ///在项目中是没有父对象的，所以这里是什么都没有的空函数
  virtual void modifyParent(const Project* in_project, const Metadata* in_old_parent) const;

  virtual void modifyParent(const Shot* in_shot, const Metadata* in_old_parent) const;
  virtual void modifyParent(const Episodes* in_episodes, const Metadata* in_old_parent) const;
  virtual void modifyParent(const Assets* in_assets, const Metadata* in_old_parent) const;
  virtual void modifyParent(const AssetsFile* in_assetsFile, const Metadata* in_old_parent) const;

 private:
  void modifyParent(const Metadata* in_metadata, const Metadata* in_old_parent) const;
  void loadChild(Metadata* in_metadata, const FSys::path& k_config) const;
  FSys::path getRoot(const Metadata* in_metadata) const;
  void save(const Metadata* in_metadata, const FSys::path& in_path) const;
};

}  // namespace doodle
