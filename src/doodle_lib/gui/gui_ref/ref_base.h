#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/signals2.hpp>

namespace doodle::gui {
/**
 * @brief 基本的渲染基类, render(const entt::handle & )必须每帧调用
 *
 */
class DOODLELIB_API base_render {
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
  virtual void render(const entt::handle &in = {}) = 0;
};

/**
 * @brief gui 缓存类的数据
 *
 */
class gui_data {
 public:
  bool is_modify{false};
  boost::signals2::signal<void()> edited;
};

/**
 * @brief 编辑类的接口
 *
 */
class DOODLELIB_API edit_interface {
 protected:
  virtual void init_(const entt::handle &in)       = 0;
  virtual void save_(const entt::handle &in) const = 0;

  void set_modify(bool is_modify);

 public:
  edit_interface();
  ~edit_interface();

  std::unique_ptr<gui_data> data_;

  virtual void init(const entt::handle &in);
  virtual void render(const entt::handle &in) = 0;
  virtual void save(const entt::handle &in);
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
  bool select;
};

/**
 * @brief gui 额外的路径类
 *
 */
class gui_cache_path {
 public:
  FSys::path path;
};

/**
 * @brief gui 名称,使用uuid 作为id
 *
 */
class gui_cache_name_id {
 public:
  std::string name_id;
  std::string_view name;
  gui_cache_name_id()
      : gui_cache_name_id(std::string{}) {}

  explicit gui_cache_name_id(const std::string &in_name);

  const char *operator*() const noexcept;

  gui_cache_name_id(gui_cache_name_id &&in_r) noexcept;
  gui_cache_name_id &operator=(gui_cache_name_id &&in_r) noexcept;
  gui_cache_name_id(gui_cache_name_id &in_r) noexcept;
  gui_cache_name_id &operator=(gui_cache_name_id &in_r) noexcept;
};

/**
 * @brief gui缓存模板类
 *
 * @tparam T 数据
 * @tparam BaseType 基类, 可以时gui_cache_null_data gui_cache_select gui_cache_path gui_cache_name_id
 * 或者自定义 , 默认是 空类
 */
template <class T, class BaseType = gui_cache_null_data>
class gui_cache : public BaseType {
 public:
  using base_type = BaseType;
  gui_cache_name_id gui_name;
  T data;

  /**
   * @brief 构建数据
   *
   * @tparam IN_T 传入的自定义构建数据类
   * @param in_name gui 的显示名称
   * @param in_data gui 数据初始化构建
   */
  template <class IN_T>
  explicit gui_cache(const std::string &in_name, IN_T &&in_data)
      : base_type(),
        gui_name(in_name),
        data(std::forward<IN_T>(in_data)){};

  template <class IN_T, std::enable_if_t<doodle::details::is_smart_pointer<IN_T>::value, bool> = true>
  explicit gui_cache(IN_T &in_data)
      : gui_cache(fmt::to_string(*in_data), in_data) {}

  template <class IN_T, std::enable_if_t<!doodle::details::is_smart_pointer<IN_T>::value, bool> = true>
  explicit gui_cache(const IN_T &in_data)
      : gui_cache(fmt::to_string(in_data), in_data) {}

  bool operator<(const gui_cache &in_rhs) const {
    return data < in_rhs.data;
  }
  bool operator>(const gui_cache &in_rhs) const {
    return in_rhs < *this;
  }
  bool operator<=(const gui_cache &in_rhs) const {
    return !(in_rhs < *this);
  }
  bool operator>=(const gui_cache &in_rhs) const {
    return !(*this < in_rhs);
  }
  bool operator==(const gui_cache &in_rhs) const {
    return data == in_rhs.data;
  }
  bool operator!=(const gui_cache &in_rhs) const {
    return !(in_rhs == *this);
  }

  gui_cache &operator=(const T &in) {
    data = in;
    return *this;
  }
  operator T &() {
    return data;
  }
  operator const T &() const {
    return data;
  }
};
}  // namespace doodle::gui
