//
// Created by teXiao on 2021/4/27.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <cereal/types/polymorphic.hpp>
#include <cereal/types/memory.hpp>

namespace doodle {
class DOODLELIB_API Metadata : public std::enable_shared_from_this<Metadata> {
  friend MetadataFactory;
 protected:
  ///弱父对象的指针
  std::weak_ptr<Metadata> p_parent;
  ///子对象的序列
  std::vector<MetadataPtr> p_child_items;

  ///这个时文件的根名称， 基本判断相同就直接比较他俩就行
  std::string p_Root;
  ///这个名称保存时的名称（文件名称这个不影响任何判断）
  std::string p_Name;
  ///这个时父对象的root
  std::string p_parent_uuid;

  /**
   * 这个是加载或者保存时的工厂
   * 这个工厂会在加载时记录, 或者在第一次保存时记录
   * @warning 基本保证在使用时不空（从逻辑上）
   */
  MetadataFactoryPtr p_metadata_flctory_ptr_;


  virtual bool sort(const Metadata &in_rhs) const = 0;
  virtual void modifyParent(const std::shared_ptr<Metadata>& in_old_parent) = 0;

 public:
  Metadata();
  ///这个时直接创建对象的，其中会自动设置父指针
  /**
   * @param in_metadata 父指针输入
   */
  explicit Metadata(std::weak_ptr<Metadata> in_metadata);
  virtual ~Metadata();
  ///设置父指针
  [[nodiscard]] virtual bool hasParent() const;
  ///活动父指针
  [[nodiscard]] virtual std::shared_ptr<Metadata> getParent() const;

  ///设置父指针
  virtual void setParent(const std::shared_ptr<Metadata> &in_parent);

  ///询问是否有孩子
  [[nodiscard]] virtual bool hasChild() const;
  ///获得孩子
  /**
   * @return 孩子的列表
   */
  [[nodiscard]] virtual const std::vector<MetadataPtr> &getChildItems() const;
  ///清除所有孩子
  virtual void clearChildItems();
  ///去除其中一个孩子
  virtual bool removeChildItems(const MetadataPtr &in_child);
  ///设置所有孩子
  virtual void setChildItems(const std::vector<MetadataPtr> &in_child_items);
  ///添加一个孩子
  virtual void addChildItem(const MetadataPtr &in_items);
  ///排序一个孩子
  virtual void sortChildItems();

  ///这里时转换为字符串的, 这里不可以有中文
  /**
   * @return 没有中文的字符串
   */
  [[nodiscard]] virtual std::string str() const = 0;
  ///这里时显示的字符串, 极有可能有中文
  /**
   * @return 有或者没有中文的字符串, 但是意思一定时很明了的
   */
  [[nodiscard]] virtual std::string showStr() const;

  ///获得根uuid
  /**
   * @return 根uuid
   */
  [[nodiscard]] virtual const std::string &getRoot() const;
  [[nodiscard]] virtual const std::string &getRoot();
  ///获得名称,这个名称是文件名称
  /**
   * @return
   */
  [[nodiscard]] virtual const std::string &getName() const;
  [[nodiscard]] virtual const std::string &getName();
  ///这个会一直递归找到没有父级的根节点
  /**
   * @return 根节点(现在基本上是项目节点)
   */
  [[nodiscard]] const MetadataPtr getRootParent();
  virtual void createMenu(ContextMenu* in_contextMenu) = 0;
  //  [[nodiscard]] virtual FSys::path FolderPath() const;

  ///获得序列化他们的工厂
  /**
   * @return
   */
  const MetadataFactoryPtr &getMetadataFactory() const;

  ///检查父亲是否符合记录
  /**
   * @param in_metadata 输入父亲
   * @return 返回是否是这个的父亲
   */
  [[nodiscard]] virtual bool checkParent(const Metadata &in_metadata) const;

  /**
   * @warning 此处如果进行比较， 会自动转化为子类进行比较， 相同子类优化， 不同子类字符串比较
   */
  virtual bool operator<(const Metadata &in_rhs) const;
  virtual bool operator>(const Metadata &in_rhs) const;
  virtual bool operator<=(const Metadata &in_rhs) const;
  virtual bool operator>=(const Metadata &in_rhs) const;

  ///这里是使用工厂进行加载和保存的函数
  /**
   * 使用访问者模式
   * @param in_factory 序列化工厂
   */
  virtual void load(const MetadataFactoryPtr &in_factory) = 0;
  /**
   *
   * @param in_factory 序列化工厂
   */
  virtual void save(const MetadataFactoryPtr &in_factory) = 0;
  /**
   * 删除这个数据
   * @param in_factory 序列化工厂
   */
  virtual void deleteData(const MetadataFactoryPtr& in_factory) =0;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);
};

template <class Archive>
void Metadata::serialize(Archive &ar, std::uint32_t const version) {
  if (version == 1)
    ar(
        cereal::make_nvp("UUID_Root", p_Root),
        cereal::make_nvp("UUID_name", p_Name),
        cereal::make_nvp("UUID_parent", p_parent_uuid));
}
}  // namespace doodle
CEREAL_REGISTER_TYPE(doodle::Metadata)
// CEREAL_REGISTER_POLYMORPHIC_RELATION(std::enable_shared_from_this<doodle::Metadata>, doodle::Metadata)
CEREAL_CLASS_VERSION(doodle::Metadata, 1)
