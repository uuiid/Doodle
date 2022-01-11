//
// Created by TD on 2021/5/7.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <ostream>
namespace doodle {

/**
 * 父亲负责加载孩子，在加载父亲时，孩子会同时加载完成， 这样， 父亲会知道是否加载玩成孩子
 * 在保存时，保存路径是： prjRoot /父亲的root（uuid）/孩子的（root）uuid（文件）
 * 这样，我们就形成了逻辑闭环
 * 同时， 我们的父亲加载孩子时还会验证孩子的父子是否一致
 *
 * @warning 我们在更改父亲时， 要同时移动文件和保存更改
 */
class DOODLELIB_API metadata_serialize : public std::enable_shared_from_this<metadata_serialize> {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  metadata_serialize();
  ~metadata_serialize();

  virtual std::vector<entt::entity> get_all_prj() const;
  virtual bool insert_into(entt::entity in) const;
  virtual void delete_data(entt::entity in) const;
  /**
   * @brief 在这里测试使用具有父级， 并且如果有父级， 还要更新父id， 那么就可以断定也要更新父级的记录
   * @param in_metadata
   */
  virtual void updata_db(entt::entity in) const;
  virtual void select_indb(entt::entity in) const;

  virtual void select_indb_by_root(entt::entity in_root) const;
};

}  // namespace doodle
