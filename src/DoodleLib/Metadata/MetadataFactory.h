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
  RpcClientPtr p_rpcClien;

 public:
  MetadataFactory();

  virtual std::vector<ProjectPtr> getAllProject() const;

  virtual void select_indb(Project* in_project) const;
  virtual void select_indb(Shot* in_shot) const;
  virtual void select_indb(Episodes* in_episodes) const;
  virtual void select_indb(Assets* in_assets) const;
  virtual void select_indb(AssetsFile* in_assetsFile) const;

  virtual bool insert_into(Project* in_metadata) const;
  virtual bool insert_into(Shot* in_metadata) const;
  virtual bool insert_into(Episodes* in_metadata) const;
  virtual bool insert_into(Assets* in_metadata) const;
  virtual bool insert_into(AssetsFile* in_metadata) const;

  virtual void updata_db(Project* in_project) const;
  virtual void updata_db(Shot* in_shot) const;
  virtual void updata_db(Episodes* in_episodes) const;
  virtual void updata_db(Assets* in_assets) const;
  virtual void updata_db(AssetsFile* in_assetsFile) const;

  virtual void deleteData(const Project* in_metadata) const;
  virtual void deleteData(const Shot* in_metadata) const;
  virtual void deleteData(const Episodes* in_metadata) const;
  virtual void deleteData(const Assets* in_metadata) const;
  virtual void deleteData(const AssetsFile* in_metadata) const;

  virtual bool hasChild(const Metadata* in_metadata) const;

 private:
  virtual bool insert_into(Metadata* in_metadata) const;
  virtual void deleteData(const Metadata* in_metadata) const;
  virtual void updata_db(MetadataPtr& in_metadata) const;
  virtual void select_indb(MetadataPtr& in_metadata) const;
};

}  // namespace doodle
