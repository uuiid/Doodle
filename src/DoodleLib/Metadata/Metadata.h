//
// Created by teXiao on 2021/4/27.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/core/observable_container.h>
#include <DoodleLib/libWarp/protobuf_warp.h>

#include <any>
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
class DOODLELIB_API Metadata
    : public std::enable_shared_from_this<Metadata>,
      public details::no_copy {
 public:
  /**
   * @brief 这个枚举是数据库中的枚举， 更改请慎重
   * 
   */
  enum class meta_type {
    unknown_file       = 0,
    project_root       = 1,
    file               = 2,
    folder             = 3,
    derive_file        = 4,
    animation_lib_root = 5
  };

 private:
  friend MetadataFactory;
  friend RpcMetadataClient;
  friend RpcMetadaataServer;
  /// 需要加载
  bool p_need_save;
  /// 需要保存
  bool p_need_load;
  bool p_updata_parent_id;
  bool p_updata_type;

  uint64_t p_has_child;

 protected:
  void install_slots();

  ///弱父对象的指针
  std::weak_ptr<Metadata> p_parent;

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
  meta_type p_type;

  bool child_item_is_sort;

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
   * @brief 这里是使用工厂进行加载和保存的函数
   * 使用访问者模式
   * @warning 注意,这里进行工厂加载是不触发任何的添加子物体和子物体更改等任何插槽的，
   *   工厂在添加子物体时应该调用 observable_container:: 中不带_sig后缀的方法
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
  /**
   * @brief 这个时直接创建对象的，其中会自动设置父指针
   * @param in_metadata 父指针输入
   */
  explicit Metadata(std::weak_ptr<Metadata> in_metadata);
  virtual ~Metadata();

  observable_container<std::vector<MetadataPtr>> child_item;
  std::any user_date;

  void add_child(const MetadataPtr &val);

  /**
   * @brief 有父 这个是判断有父指针并且已加载父物体
   * @return
   */
  [[nodiscard]] virtual bool hasParent() const;
  [[nodiscard]] virtual bool has_parent_id() const;
  [[nodiscard]] virtual std::uint64_t get_parent_id() const;
  /**
   * @brief 活动父指针
   * @return
   */
  [[nodiscard]] virtual std::shared_ptr<Metadata> getParent() const;
  /**
   * @brief 这个时查询是否具有子项的(具有复杂的逻辑)
   * 
   * @return true 有子项
   * @return false 工厂和列表中均不具有子项
   */
  [[nodiscard]] virtual bool hasChild() const;

  virtual void sortChildItems(bool is_launch_sig = false);  ///< 排序一个孩子

  /**
   * @return 没有中文的字符串
   */
  [[nodiscard]] virtual std::string str() const = 0;  ///< 这里时转换为字符串的, 这里不可以有中文

  /**
   * @return 有或者没有中文的字符串, 但是意思一定时很明了的
   */
  [[nodiscard]] virtual std::string showStr() const;  ///< 这里时显示的字符串, 极有可能有中文

  [[nodiscard]] const std::string &getUUID() const;  ///< 获得uuid
  [[nodiscard]] FSys::path getUrlUUID() const;       ///< 这个是获得所属项目的保持相对路径

  uint64_t getId() const;  ///< 获得数据库id
  /**
   * 获得字符串id
   * @return id的字符串形式
   */
  inline std::string getIdStr() const {
    return std::to_string(getId());
  };

  /**
   * @brief 设置数据库中的类型
   * 
   * @param in_meta 类型
   */
  void set_meta_typp(const meta_type &in_meta);
  void set_meta_typp(const std::string &in_meta);
  void set_meta_type(std::int32_t in_);
  /**
   * @brief 获得数据库中的类型
   * 
   * @return 数据库中的类型 
   */
  meta_type get_meta_type() const;
  std::string get_meta_type_str() const;
  std::int32_t get_meta_type_int() const;

  /**
   * @brief  这个会一直递归找到没有父级的根节点
   * @return 根节点(现在基本上是项目节点)
   */
  [[nodiscard]] MetadataConstPtr getRootParent() const;

  //  [[nodiscard]] virtual FSys::path FolderPath() const;

  /**
   * @brief 获得序列化他们的工厂
   * @return
   */
  const MetadataFactoryPtr &getMetadataFactory() const;

  /**
   * @brief 检查父亲是否符合记录
   * @param in_metadata 输入父亲
   * @return 返回是否是这个的父亲
   */
  [[nodiscard]] virtual bool checkParent(const Metadata &in_metadata) const;

  virtual void create_menu(const menu_factory_ptr &in_factoryPtr) = 0;

  template <class parent_class>
  std::shared_ptr<parent_class> find_parent_class() {
    auto k_m = this->shared_from_this();
    while (k_m->hasParent()) {
      if (details::is_class<parent_class>(k_m))
        return std::dynamic_pointer_cast<parent_class>(k_m);
      else
        k_m = k_m->getParent();
    }
    return {};
  };
  template <class parent_class>
  std::string find_parent_class_to_string() {
    auto k_m = find_parent_class<parent_class>();
    return k_m ? k_m->showStr() : std::string{};
  };
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
   *  工厂在添加子物体时应该调用 observable_container:: 中不带_sig后缀的方法
   * @param in_factory 序列化工厂
   */
  virtual void select_indb(const MetadataFactoryPtr &in_factory = {});
  /**
   *
   * @param in_factory 序列化工厂
   */
  virtual void updata_db(const MetadataFactoryPtr &in_factory = {});
  /**
   * @brief  删除这个数据
   * @param in_factory 序列化工厂
   */
  virtual void deleteData(const MetadataFactoryPtr &in_factory = {});
  /**
   * @brief 插入函数
   * 
   * @param in_factory 
   */
  virtual void insert_into(const MetadataFactoryPtr &in_factory = {});

  boost::signals2::signal<void()> sig_change;

  virtual void to_DataDb(DataDb &in_) const;
  static MetadataPtr from_DataDb(const DataDb &in_);
  explicit operator DataDb() const;

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);
};

template <class Archive>
void Metadata::serialize(Archive &ar, std::uint32_t const version) {
  //  p_has_child = child_item.size();
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
