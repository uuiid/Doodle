//
// Created by teXiao on 2021/4/27.
//

#pragma once
#include <doodle_lib/core/observable_container.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/protobuf_warp.h>
#include <doodle_lib/metadata/metadata_factory.h>
#include <doodle_lib/metadata/tree_adapter.h>

#include <any>
#include <boost/intrusive/intrusive_fwd.hpp>
#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/pack_options.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/trivial_value_traits.hpp>
#include <boost/serialization/export.hpp>
#include <boost/signals2.hpp>
#include <optional>
namespace doodle {
template <class Class, class factory>
class database_action {
 public:
  using factory_ptr = std::shared_ptr<factory>;

 private:
  /**
   * @brief 需要加载
   */
  bool p_need_load;
  /**
   * @brief 需要保存
   */
  bool p_need_save;

 protected:
  /**
   * 这个是加载或者保存时的工厂
   * 这个工厂会在加载时记录, 或者在第一次保存时记录
   * 同时在添加子物体时也会继承父级的工厂,在整个项目中工厂应该保持一致
   * @warning 基本保证在使用时不空（从逻辑上）
   */
  factory_ptr p_factory;
  ///这个时文件的根名称， 基本判断相同就直接比较他俩就行

  std::uint64_t p_id;

  Class *metadata_self;
  [[nodiscard]] bool is_saved() const {
    return !p_need_save;
  };
  [[nodiscard]] bool is_loaded() const {
    return !p_need_load;
  };

 public:
  explicit database_action(Class *in_ptr)
      : metadata_self(in_ptr),
        p_factory(),
        p_need_load(true),
        p_need_save(true),
        p_id(0){};

  std::uint64_t getId() const {
    return p_id;
  };  ///< 获得数据库id

  /**
   * @brief 添加需要保存的状态
   *
   * @param in_need 需要保存
   */
  void saved(bool in_need = false) {
    p_need_save = in_need;
  };
  /**
   * @brief 添加需要加载的状态
   *
   * @param in_need 需要加载
   */
  void loaded(bool in_need = false) {
    p_need_load = in_need;
  };

  [[nodiscard]] bool is_install() const {
    return p_id > 0;
  };

  /**
   * @brief 这里是使用工厂进行加载和保存的函数
   * 使用访问者模式
   * @warning 注意,这里进行工厂加载是不触发任何的添加子物体和子物体更改等任何插槽的，
   *  工厂在添加子物体时应该调用 observable_container:: 中不带_sig后缀的方法
   * @param in_factory 序列化工厂
   */
  void select_indb(const factory_ptr &in_factory = {}) {
    if (in_factory)
      p_factory = in_factory;
    if (is_loaded())
      return;

    p_factory->select_indb(metadata_self);
    loaded();
    saved();
  };
  /**
   *
   * @param in_factory 序列化工厂
   */
  void updata_db(const factory_ptr &in_factory = {}) {
    if (in_factory)
      p_factory = in_factory;

    if (is_saved())
      return;

    if (is_install())
      p_factory->updata_db(metadata_self);
    else
      insert_into(p_factory);

    saved();
    loaded();
  };
  /**
   * @brief  删除这个数据
   * @param in_factory 序列化工厂
   */
  void deleteData(const factory_ptr &in_factory = {}) {
    if (in_factory)
      p_factory = in_factory;

    p_factory->delete_data(metadata_self);
  };
  /**
   * @brief 插入函数
   *
   * @param in_factory 序列化工厂
   */
  void insert_into(const factory_ptr &in_factory = {}) {
    if (in_factory)
      p_factory = in_factory;
    p_factory->insert_into(metadata_self);
    saved();
    loaded();
  };
};

// template <class Class>
// class menu_create {
//   Class *menu_self;
//
//  public:
//   explicit menu_create(Class *in_ptr)
//       : menu_self(in_ptr);
// };

/**
 * @warning 这里这个基类是不进行cereal注册的要不然会序列化出错
 * @TODO: 将父子关系函数进行提取出超类
 */
class DOODLELIB_API metadata
    : public std::enable_shared_from_this<metadata>,
      public database_action<metadata, metadata_factory> {
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
  friend metadata_factory;
  friend rpc_metadata_client;
  friend rpc_metadaata_server;

  bool p_updata_parent_id;
  bool p_updata_type;

  std::size_t p_has_child;
  std::size_t p_has_file;

 protected:
  ///弱父对象的指针
  std::weak_ptr<metadata> p_parent;

  ///这个时父对象的root
  std::optional<uint64_t> p_parent_id;

  std::string p_uuid;
  meta_type p_type;

  bool child_item_is_sort;
  friend vector_adapter<std::vector<metadata_ptr>, metadata>;
  void end_push_back(const metadata_ptr &in_val);
  void end_erase(const metadata_ptr &in_val);
  void end_clear();

 public:
  metadata();
  /**
   * @brief 这个时直接创建对象的，其中会自动设置父指针
   * @param in_metadata 父指针输入
   */
  explicit metadata(std::weak_ptr<metadata> in_metadata);
  virtual ~metadata();

  std::vector<metadata_ptr> child_item;

  void add_child(const metadata_ptr &val);

  inline vector_adapter<std::vector<metadata_ptr>, metadata> get_child() {
    return make_vector_adapter(child_item, *this);
  };
  /**
   * @brief 有父 这个是判断有父指针并且已加载父物体
   * @return
   */
  [[nodiscard]] virtual bool has_parent() const;
  [[nodiscard]] virtual bool has_parent_id() const;
  [[nodiscard]] virtual std::uint64_t get_parent_id() const;
  /**
   * @brief 活动父指针
   * @return
   */
  [[nodiscard]] virtual std::shared_ptr<metadata> get_parent() const;
  /**
   * @brief 这个时查询是否具有子项的(具有复杂的逻辑)
   *
   * @return true 有子项
   * @return false 工厂和列表中均不具有子项
   */
  [[nodiscard]] virtual bool has_child() const;
  [[nodiscard]] virtual bool has_file() const;

  virtual void sort_child_items(bool is_launch_sig = false);  ///< 排序一个孩子

  /**
   * @return 没有中文的字符串
   */
  [[nodiscard]] virtual std::string str() const = 0;  ///< 这里时转换为字符串的, 这里不可以有中文

  /**
   * @return 有或者没有中文的字符串, 但是意思一定时很明了的
   */
  [[nodiscard]] virtual std::string show_str() const;  ///< 这里时显示的字符串, 极有可能有中文

  [[nodiscard]] const std::string &get_uuid() const;  ///< 获得uuid
  [[nodiscard]] FSys::path get_url_uuid() const;      ///< 这个是获得所属项目的保持相对路径

  /**
   * 获得字符串id
   * @return id的字符串形式
   */
  inline std::string get_id_str() const {
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
  [[nodiscard]] metadata_const_ptr get_root_parent() const;

  /**
   * @brief 获得序列化他们的工厂
   * @return
   */
  const metadata_factory_ptr &get_metadata_factory() const;

  /**
   * @brief 检查父亲是否符合记录
   * @param in_metadata 输入父亲
   * @return 返回是否是这个的父亲
   */
  [[nodiscard]] virtual bool check_parent(const metadata &in_metadata) const;

  virtual void attribute_widget(const attribute_factory_ptr &in_factoryPtr) = 0;

  template <class parent_class>
  std::shared_ptr<parent_class> find_parent_class() {
    auto k_m = this->shared_from_this();
    while (k_m) {
      if (details::is_class<parent_class>(k_m))
        return std::dynamic_pointer_cast<parent_class>(k_m);
      else {
        if (k_m->has_parent())
          k_m = k_m->get_parent();
        else
          k_m = nullptr;
      }
    }
    return {};
  };

  template <class parent_class>
  std::shared_ptr<const parent_class> find_parent_class() const {
    auto k_m = this->shared_from_this();
    while (k_m) {
      if (details::is_class<const parent_class>(k_m))
        return std::dynamic_pointer_cast<const parent_class>(k_m);
      else {
        if (k_m->has_parent())
          k_m = k_m->get_parent();
        else
          k_m = nullptr;
      }
    }
    return {};
  };

  template <class parent_class>
  std::string find_parent_class_to_string() {
    auto k_m = find_parent_class<parent_class>();
    return k_m ? k_m->show_str() : std::string{};
  };
  /**
   * @warning 此处如果进行比较， 会自动转化为子类进行比较， 相同子类优化， 不同子类字符串比较
   */
  virtual bool operator<(const metadata &in_rhs) const;
  virtual bool operator>(const metadata &in_rhs) const;
  virtual bool operator<=(const metadata &in_rhs) const;
  virtual bool operator>=(const metadata &in_rhs) const;
  bool operator==(const metadata &in_rhs) const;
  bool operator!=(const metadata &in_rhs) const;

  virtual void to_DataDb(metadata_database &in_) const;
  static metadata_ptr from_DataDb(const metadata_database &in_);
  explicit operator metadata_database() const;

  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, const std::uint32_t version);
};

template <class Archive>
void metadata::serialize(Archive &ar, const std::uint32_t version) {
  if (version == 3) {
    ar &BOOST_SERIALIZATION_NVP(p_id);
    ar &BOOST_SERIALIZATION_NVP(p_parent_id);
    ar &BOOST_SERIALIZATION_NVP(p_uuid);
    ar &BOOST_SERIALIZATION_NVP(p_has_child);
    ar &BOOST_SERIALIZATION_NVP(p_has_file);
  }
}

class DOODLELIB_API tree_relationship
/* : public boost::intrusive::set_base_hook<> */ {
 private:
  entt::entity p_parent;

 public:
  std::vector<entt::entity> p_child;
  // boost::intrusive::set<> p_child;

  tree_relationship::tree_relationship()
      : p_parent(entt::null),
        p_child() {
  }
  tree_relationship(entt::entity in_parent)
      : tree_relationship() {
    set_parent(std::move(in_parent));
  }
  bool has_parent() const {
    return p_parent != entt::null;
  };

  [[nodiscard]] const entt::entity &get_parent() const noexcept;
  void set_parent(const entt::entity &in_parent) noexcept;

  [[nodiscard]] const std::vector<entt::entity> &get_child() const noexcept;
  [[nodiscard]] std::vector<entt::entity> &get_child() noexcept;
  void set_child(const std::vector<entt::entity> &in_child) noexcept;

  entt::entity get_root() const;
};

class DOODLELIB_API database {
 private:
  std::uint64_t p_id;
  std::optional<uint64_t> p_parent_id;
  metadata::meta_type p_type;
  std::string p_uuid;
  std::uint32_t p_boost_serialize_vesion;

 public:
  database();
  ~database();

  std::size_t p_has_child;
  std::size_t p_has_file;
  bool p_updata_parent_id;
  bool p_updata_type;
  /**
   * @brief 需要加载
   */
  bool p_need_load;
  /**
   * @brief 需要保存
   */
  bool p_need_save;

  FSys::path get_url_uuid() const;
  bool has_parent() const;
  std::int32_t get_meta_type_int() const;

  /**
   * @brief 设置数据库中的类型
   *
   * @param in_meta 类型
   */
  void set_meta_typp(const metadata::meta_type &in_meta);
  void set_meta_typp(const std::string &in_meta);
  void set_meta_type(std::int32_t in_);


  database &operator=(const metadata_database &in);
  explicit operator metadata_database() const;

  friend class boost::serialization::access;
  template <class Archive>
  void save(Archive &ar, const std::uint32_t version) const {
    ar &BOOST_SERIALIZATION_NVP(p_id);
    ar &BOOST_SERIALIZATION_NVP(p_parent_id);
    ar &BOOST_SERIALIZATION_NVP(p_type);
    ar &BOOST_SERIALIZATION_NVP(p_uuid);
    ar &BOOST_SERIALIZATION_NVP(p_has_child);
    ar &BOOST_SERIALIZATION_NVP(p_has_file);
  };

  template <class Archive>
  void load(Archive &ar, const std::uint32_t version) {
    p_boost_serialize_vesion = version;
    if (version == 1) {
      ar &BOOST_SERIALIZATION_NVP(p_id);
      ar &BOOST_SERIALIZATION_NVP(p_parent_id);
      ar &BOOST_SERIALIZATION_NVP(p_type);
      ar &BOOST_SERIALIZATION_NVP(p_uuid);
      ar &BOOST_SERIALIZATION_NVP(p_has_child);
      ar &BOOST_SERIALIZATION_NVP(p_has_file);
    }
  };

  BOOST_SERIALIZATION_SPLIT_MEMBER()
};
}  // namespace doodle

// CEREAL_REGISTER_TYPE(doodle::metadata)
// CEREAL_REGISTER_POLYMORPHIC_RELATION(std::enable_shared_from_this<doodle::metadata>, doodle::metadata)
BOOST_CLASS_VERSION(doodle::metadata, 3)
BOOST_SERIALIZATION_ASSUME_ABSTRACT(doodle::metadata)
BOOST_CLASS_EXPORT_KEY(doodle::metadata)

BOOST_CLASS_VERSION(doodle::database, 1)
BOOST_SERIALIZATION_ASSUME_ABSTRACT(doodle::database)
BOOST_CLASS_EXPORT_KEY(doodle::database)
