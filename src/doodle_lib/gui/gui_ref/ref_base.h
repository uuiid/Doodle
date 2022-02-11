#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui {

class DOODLELIB_API base_render {
 public:
  base_render() = default;
  virtual bool operator()(const entt::handle &in_handle){};
};

template <class T>
base_render make_render() {
}

class base_edit {
 protected:
  virtual void init_(const entt::handle &in)       = 0;
  virtual void save_(const entt::handle &in) const = 0;

 public:
  bool is_modify{false};
  void init(const entt::handle &in);
  virtual void render(const entt::handle &in) = 0;
  void save(const entt::handle &in);
};

namespace details {
template <class T>
class gui_cache {
 public:
  T data;
  std::string name;
  explicit gui_cache(const T &in_data, const std::string &in_name)
      : data(in_data),
        name(fmt::format("{}##{}", in_name, fmt::ptr(this))){};

  explicit gui_cache(const T &in_data)
      : gui_cache(in_data, fmt::to_string(in_data)) {}

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

  constexpr operator T &() {
    return data;
  }
  constexpr operator const T &() const {
    return data;
  }
};
}  // namespace details

}  // namespace doodle::gui
