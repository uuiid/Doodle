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
class DOODLELIB_API MetadataFactory : public std::enable_shared_from_this<MetadataFactory> {
  std::weak_ptr<RpcMetadataClient> p_rpcClien;

 public:
  MetadataFactory();

  virtual std::vector<ProjectPtr> getAllProject();

  virtual bool insert_into(Metadata* in_metadata) const;
  virtual void deleteData(const Metadata* in_metadata) const;
  virtual void updata_db(Metadata* in_metadata) const;
  virtual void select_indb(Metadata* in_metadata) const;
};

}  // namespace doodle
