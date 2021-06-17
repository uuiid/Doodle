//
// Created by teXiao on 2021/4/27.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <boost/signals2.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/polymorphic.hpp>
#include <optional>
namespace doodle {
/**
 * @warning 这里这个基类是不进行cereal注册的要不然会序列化出错
 *
 */
class DOODLELIB_API Metadata : public std::enable_shared_from_this<Metadata>, public details::no_copy {
  friend MetadataFactory;
  friend RpcMetadataClient;
  friend RpcMetadaataServer;
  /// 需要加载
  bool p_need_save;
  /// 需要保存
  bool p_need_load;
  bool p_updata_parent_id;

  uint64_t p_has_child;

 protected:
  ///弱父对象的指针
  std::weak_ptr<Metadata> p_parent;
  ///子对象的序列
  std::vector<MetadataPtr> p_child_items;

  ///这个时文件的根名称， 基本判断相同就直接比较他俩就行
  uint64_t p_id;
  ///这个时父对象的root
  std::optional<uint64_t> p_parent_id;

  std::string p_uuid;
  /**
   * 这个是加载或者保存时的工厂
   * 这个工厂会在加载时记录, 或者在第一次保存时记录
   * 同时在添加子物体时也会继承父级的工厂,在整个项目中工厂应该保持一致
   * @warning 基本保证在使用时不空（从逻辑上）
   */
  MetadataFactoryPtr p_metadata_flctory_ptr_;

  inline bool isInstall() const { return p_id > 0; };

  virtual bool sort(const Metadata &in_rhs) const = 0;

  /// 设置为已经加载
  virtual void loaded(bool in_need = false);
  /// 设置为已经保存
  virtual void saved(bool in_need = false);

  /// 是已经加载过的
  virtual bool isLoaded() const;
  /// 是已经保存过的
  virtual bool isSaved() const;
  /**
   * @brief 这个添加子物体时不会触发信号
   * 
   * @param in_items 子物体
   */
  virtual void addChildItemNotSig(const MetadataPtr &in_items);

  /**
   * @brief 这里是使用工厂进行加载和保存的函数
   * 使用访问者模式
   * @warning 注意,这里进行工厂加载是不触发任何的添加子物体和子物体更改等任何插槽的，
   *  工厂在添加子物体时应该调用 Metadata::addChildItemNotSig(const MetadataPtr &) 方法
   * @param in_factory 序列化工厂
   */
  virtual void _select_indb(const MetadataFactoryPtr &in_factory) = 0;
  /**
   *
   * @param in_factory 序列化工厂
   */
  virtual void _updata_db(const MetadataFactoryPtr &in_factory) = 0;
  /**
   * @brief  删除这个数据
   * @param in_factory 序列化工厂
   */
  virtual void _deleteData(const MetadataFactoryPtr &in_factory) = 0;
  /**
   * @brief 插入函数
   * 
   * @param in_factory 
   */
  virtual void _insert_into(const MetadataFactoryPtr &in_factory) = 0;

 public:
  Metadata();
  ///这个时直接创建对象的，其中会自动设置父指针
  /**
   * @param in_metadata 父指针输入
   */
  explicit Metadata(std::weak_ptr<Metadata> in_metadata);
  virtual ~Metadata();

  [[nodiscard]] virtual bool hasParent() const;  ///< 设置父指针

  [[nodiscard]] virtual std::shared_ptr<Metadata> getParent() const;  ///< 活动父指针
  /**
   * @brief 这个时查询是否具有子项的(具有复杂的逻辑)
   * 
   * @return true 有子项
   * @return false 工厂和列表中均不具有子项
   */
  [[nodiscard]] virtual bool hasChild() const;

  /**
   * @return 孩子的列表
   */
  [[nodiscard]] virtual const std::vector<MetadataPtr> &getChildItems() const;  ///< 获得孩子

  virtual void clearChildItems();  ///< 清除所有孩子

  virtual bool removeChildItems(const MetadataPtr &in_child);  ///< 去除其中一个孩子

  virtual void setChildItems(const std::vector<MetadataPtr> &in_child_items);  ///< 设置所有孩子

  virtual MetadataPtr addChildItem(const MetadataPtr &in_items);  ///< 添加一个孩子

  virtual void sortChildItems();  ///< 排序一个孩子

  /**
   * @return 没有中文的字符串
   */
  [[nodiscard]] virtual std::string str() const = 0;  ///< 这里时转换为字符串的, 这里不可以有中文

  /**
   * @return 有或者没有中文的字符串, 但是意思一定时很明了的
   */
  [[nodiscard]] virtual std::string showStr() const;  ///< 这里时显示的字符串, 极有可能有中文

  [[nodiscard]] const std::string &getUUID();   ///< 获得uuid
  [[nodiscard]] FSys::path getUrlUUID() const;  ///< 这个是获得所属项目的保持相对路径

  uint64_t getId() const;  ///< 获得数据库id

  /**
   * @brief  这个会一直递归找到没有父级的根节点
   * @return 根节点(现在基本上是项目节点)
   */
  [[nodiscard]] MetadataConstPtr getRootParent() const;

  virtual void createMenu(ContextMenu *in_contextMenu) = 0;
  //  [[nodiscard]] virtual FSys::path FolderPath() const;

  /**
   * @return
   */
  const MetadataFactoryPtr &getMetadataFactory() const;  ///< 获得序列化他们的工厂

  /**
   * @param in_metadata 输入父亲
   * @return 返回是否是这个的父亲
   */
  [[nodiscard]] virtual bool checkParent(const Metadata &in_metadata) const;  ///< 检查父亲是否符合记录

  ///本身进行更改时发出信号
  boost::signals2::signal<
      void()>
      sig_thisChange;
  ///清除孩子时发出信号
  boost::signals2::signal<
      void()>
      sig_childClear;

  ///添加孩子是发出信号,添加孩子发出的信号会比孩子更改父级发出的晚
  boost::signals2::signal<
      void(const MetadataPtr &child_ptr)>
      sig_childAdd;
  ///整体替换时发出信号
  boost::signals2::signal<
      void(const std::vector<MetadataPtr> &child_ptr)>
      sig_childAddAll;
  ///孩子删除时发出信号
  boost::signals2::signal<
      void(const MetadataPtr &child_ptr)>
      sig_childDelete;
  /**
   * @warning 此处如果进行比较， 会自动转化为子类进行比较， 相同子类优化， 不同子类字符串比较
   */
  virtual bool operator<(const Metadata &in_rhs) const;
  virtual bool operator>(const Metadata &in_rhs) const;
  virtual bool operator<=(const Metadata &in_rhs) const;
  virtual bool operator>=(const Metadata &in_rhs) const;
  bool operator==(const Metadata &in_rhs) const;
  bool operator!=(const Metadata &in_rhs) const;
  /**
   * @brief 这里是使用工厂进行加载和保存的函数
   * 使用访问者模式
   * @warning 注意,这里进行工厂加载是不触发任何的添加子物体和子物体更改等任何插槽的，
   *  工厂在添加子物体时应该调用 Metadata::addChildItemNotSig(const MetadataPtr &) 方法
   * @param in_factory 序列化工厂
   */
  virtual void select_indb(const MetadataFactoryPtr &in_factory);
  /**
   *
   * @param in_factory 序列化工厂
   */
  virtual void updata_db(const MetadataFactoryPtr &in_factory);
  /**
   * @brief  删除这个数据
   * @param in_factory 序列化工厂
   */
  virtual void deleteData(const MetadataFactoryPtr &in_factory);
  /**
   * @brief 插入函数
   * 
   * @param in_factory 
   */
  virtual void insert_into(const MetadataFactoryPtr &in_factory);

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);
};

template <class Archive>
void Metadata::serialize(Archive &ar, std::uint32_t const version) {
  if (version == 1)
    ar(
        cereal::make_nvp("id", p_id),
        cereal::make_nvp("parent_id", p_parent_id),
        cereal::make_nvp("UUID", p_uuid),
        cereal::make_nvp("has_child", p_has_child));
}
}  // namespace doodle

//CEREAL_REGISTER_TYPE(doodle::Metadata)
//CEREAL_REGISTER_POLYMORPHIC_RELATION(std::enable_shared_from_this<doodle::Metadata>, doodle::Metadata)
CEREAL_CLASS_VERSION(doodle::Metadata, 1)
