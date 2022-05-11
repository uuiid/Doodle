//
// Created by TD on 2022/5/11.
//

#pragma once
#include <filesystem>
namespace doodle::FSys {
using namespace std::filesystem;
class path_u8 : public path {
 public:
  inline static std::locale locale_u8{};
  template <class Source>
  path_u8(const Source& source, format fmt = auto_format)
      : path(source, locale_u8){};
  template <class InputIt>
  path_u8(InputIt first, InputIt last, format fmt = auto_format)
      : path(first, last, locale_u8){};

  using path::path;

  ~path_u8() = default;

  // assignments

  template <class Source>
  path_u8& operator=(const Source& source) {
    path::operator=(path_u8{source});
    return *this;
  };
  template <class Source>
  path_u8& assign(const Source& source) {
    path::assign(path_u8{source});
    return *this;
  };
  template <class InputIt>
  path_u8& assign(InputIt first, InputIt last) {
    path::assign(path_u8{first, last});
    return *this;
  };

  // appends

  template <class Source>
  path_u8& operator/=(const Source& source) {
    path::operator/=(path_u8{source});
    return *this;
  };
  template <class Source>
  path_u8& append(const Source& source) {
    path::operator/=(path_u8{source});
    return *this;
  };
  template <class InputIt>
  path_u8& append(InputIt first, InputIt last) {
    path::operator/=(path_u8{first, last});
    return *this;
  };

  // concatenation
  path_u8& operator+=(const path_u8& x) {
    path::operator+=(x);
    return *this;
  };
  template <class Source>
  path_u8& operator+=(const Source& x) {
    path::operator+=(path_u8{x});
    return *this;
  };
  template <class ECharT>
  path_u8& operator+=(ECharT x) {
    path::operator+=(path_u8{x});
    return *this;
  };
  template <class Source>
  path_u8& concat(const Source& x) {
    path::operator+=(path_u8{x});
    return *this;
  };
  template <class InputIt>
  path_u8& concat(InputIt first, InputIt last) {
    path::operator+=(path_u8{first, last});
    return *this;
  };

  // modifiers
  void swap(path_u8& rhs) noexcept {
    path::swap(rhs);
  };

  // non-member operators
  friend path_u8 operator/(const path_u8& lhs, const path_u8& rhs) {
    return operator/(lhs, rhs);
  };
};

}  // namespace doodle::FSys
