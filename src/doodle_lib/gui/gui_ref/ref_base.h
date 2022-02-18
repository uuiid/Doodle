#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/signals2.hpp>

namespace doodle::gui {

class DOODLELIB_API base_render {
 public:
  base_render() = default;
  virtual ~base_render();
  virtual void render(const entt::handle &in = {}) = 0;
};

class gui_data {
 public:
  bool is_modify{false};
  boost::signals2::signal<void()> edited;
};

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

class gui_cache_null_data {
 public:
};

class gui_cache_select {
 public:
  bool select;
};

class gui_cache_name_id {
 public:
  std::string name_id;
  std::string_view name;

  gui_cache_name_id()
      : gui_cache_name_id(std::string{}) {
  }

  explicit gui_cache_name_id(const std::string &in_name);
};

template <class T, class BaseType = gui_cache_null_data>
class gui_cache : public gui_cache_name_id, public BaseType {
 public:
  using base_type = BaseType;
  T data;

  template <class IN_T>
  explicit gui_cache(const std::string &in_name, IN_T &&in_data)
      : gui_cache_name_id(in_name),
        base_type(),
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
