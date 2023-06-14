#pragma once

#include <doodle_app/doodle_app_fwd.h>

#include <boost/operators.hpp>
#include <boost/signals2.hpp>

namespace doodle::gui {
/**
 * @brief 基本的渲染基类, render(const entt::handle & )必须每帧调用
 *
 */
class DOODLE_APP_API base_render {
 public:
  /**
   * @brief Construct a new base render object
   *
   */
  base_render()                                    = default;
  /**
   * @brief Destroy the base render object
   *
   */
  virtual ~base_render()                           = default;
  /**
   * @brief 渲染函数, 每帧调用, 不要再这里添加太多的逻辑运行方法, 最好只有渲染方法
   *
   * @param in
   */
  virtual bool render(const entt::handle &in = {}) = 0;
};

/**
 * @brief 空类
 *
 */
class gui_cache_null_data {
 public:
};
/**
 * @brief 可选择的缓存类
 *
 */
class gui_cache_select {
 public:
  /// 是否选中1
  bool select;
};

/**
 * @brief gui 额外的路径类
 *
 */
class gui_cache_path {
 public:
  /// 路径数据
  FSys::path path;
};

/**
 * @brief gui 名称,使用 identifier 作为id已进行相同名称的id区分
 *
 */
class gui_cache_name_id : boost::totally_ordered<gui_cache_name_id> {
 public:
  /**
   * @brief 名称和id 的字符串 {name}##{id}
   *
   */
  std::string name_id;
  /**
   * @brief 只有名称
   * @warning 这个是std::string_view 获得的是 name_id 的数据,
   * 所以复制构造和移动构造必须小心, 不要指向错误的缓冲区
   *
   */
  std::string_view name;

  /**
   * @brief 使用空字符串进行默认构造
   *
   */
  gui_cache_name_id() : gui_cache_name_id(std::string{}) {}

  /**
   * @brief 使用传入的名称作为名称构造
   * @warning 名称中不可以使用 @b # 字符
   *
   * @param in_name 传入的名称
   */
  explicit gui_cache_name_id(const std::string &in_name);

  //  explicit gui_cache_name_id(const std::string_view &in_name) : gui_cache_name_id(std::string{in_name}){};

  /**
   * @brief 一个方便函数 直接返回字符串缓冲区指针
   *
   * @return const char* name_id指针
   */
  const char *operator*() const noexcept;

  /**
   * @brief 构造一个新的id
   * @warning 这里需要转移 name 字符串视图指向的缓冲区
   *
   * @param in_r 传入的 id
   */
  gui_cache_name_id(gui_cache_name_id &&in_r) noexcept;
  /// @copydoc gui_cache_name_id(gui_cache_name_id &&in_r)
  gui_cache_name_id &operator=(gui_cache_name_id &&in_r) noexcept;
  /// @copydoc gui_cache_name_id(gui_cache_name_id &&in_r)
  gui_cache_name_id(const gui_cache_name_id &in_r) noexcept;
  /// @copydoc gui_cache_name_id(gui_cache_name_id &&in_r)
  gui_cache_name_id &operator=(const gui_cache_name_id &in_r) noexcept;

  bool operator==(const gui_cache_name_id &in) const noexcept;
  bool operator<(const gui_cache_name_id &in) const noexcept;
};

/**
 * @brief gui缓存模板类
 *
 * @tparam T 数据
 * @tparam BaseType 基类, 可以时 gui_cache_null_data gui_cache_select gui_cache_path gui_cache_name_id
 * 或者自定义 , 默认是 gui_cache_null_data
 */
template <class T, class BaseType = gui_cache_null_data>
class gui_cache : public BaseType {
 public:
  /// 基类别名
  using base_type = BaseType;
  /// gui 名称
  gui_cache_name_id gui_name;
  /// 数据属性
  T data{};

  gui_cache() = default;

  /**
   * @brief 构建数据
   *
   * @tparam IN_T 传入的自定义构建数据类
   * @param in_name gui 的显示名称
   * @param in_data gui 数据初始化构建
   */
  template <typename... IN_T>
  explicit gui_cache(const std::string &in_name, IN_T &&...in_data)
      : base_type(), gui_name(in_name), data{std::forward<IN_T>(in_data)...} {};
  /**
   * @brief 初始化id, 并且默认构造数据
   * @param in_name
   */

  explicit gui_cache(const std::string &in_name) : base_type(), gui_name(in_name), data(){};

  /**
   * @brief 排序和比较函数， 注意， 我们只关注 data 数据， 并不会比较 gui_name
   */
  bool operator<(const gui_cache &in_rhs) const { return data < in_rhs.data; }
  /// @brief 排序和比较函数， 注意， 我们只关注 data 数据， 并不会比较 gui_name
  bool operator>(const gui_cache &in_rhs) const { return in_rhs < *this; }
  /// @brief 排序和比较函数， 注意， 我们只关注 data 数据， 并不会比较 gui_name
  bool operator<=(const gui_cache &in_rhs) const { return !(in_rhs < *this); }
  /// @brief 排序和比较函数， 注意， 我们只关注 data 数据， 并不会比较 gui_name
  bool operator>=(const gui_cache &in_rhs) const { return !(*this < in_rhs); }
  /// @brief 排序和比较函数， 注意， 我们只关注 data 数据， 并不会比较 gui_name
  bool operator==(const gui_cache &in_rhs) const { return data == in_rhs.data; }
  /// @brief 排序和比较函数， 注意， 我们只关注 data 数据， 并不会比较 gui_name
  bool operator!=(const gui_cache &in_rhs) const { return !(in_rhs == *this); }

  /**
   * @brief 方便的赋值运算符重载
   *
   * @param in 传入的数据
   * @return gui_cache&
   *
   */
  gui_cache &operator=(const T &in) {
    data = in;
    return *this;
  }
  /**
   * @brief 返回 gui id (包括显示名称)
   * @return
   */
  const char *operator*() const { return *gui_name; }
  const char *operator*() { return *gui_name; }
  /**
   * @brief 返回gui 内容
   * @return
   */
  T *operator&() { return &data; }
  const T *operator&() const { return &data; }
  /**
   * @brief 可调用返回数据(快速方法)
   * @return
   */
  T &operator()() { return data; }
  const T &operator()() const { return data; }

  /// @brief 隐式转换 T 运算符
  operator T &() { return data; }
  /// @brief 隐式转换 T 运算符
  operator const T &() const { return data; }
};
}  // namespace doodle::gui
