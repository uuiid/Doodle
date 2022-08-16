//
// Created by TD on 2021/5/28.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/detail/ios.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream_buffer.hpp>
namespace doodle {

/**
 * @brief 这个是从std 兼容的序列中读取数据的后端
 * @tparam Container 容器类
 */
template <class Container>
class container_source {
 public:
  using char_type = typename Container::value_type;
  using category  = boost::iostreams::source_tag;
  explicit container_source(Container& container)
      : container_(container), pos_(0) {}
  std::streamsize read(char_type* s, std::streamsize n) {
    using namespace std;
    auto amt =
        static_cast<std::streamsize>(container_.size() - pos_);
    std::streamsize result = (min)(n, amt);
    if (result != 0) {
      std::copy(container_.begin() + pos_,
                container_.begin() + pos_ + result,
                s);
      pos_ += result;
      return result;
    } else {
      return -1;  // EOF
    }
  }
  Container& container() { return container_; }
  container_source operator=(const container_source&) = delete;

 private:
  using size_type = typename Container::size_type;
  Container& container_;
  size_type pos_;
};

/**
 * @brief 这个是从std 兼容的序列中写入数据的后端
 * @tparam Container 容器类
 */
template <typename Container>
class container_sink {
 public:
  using char_type = typename Container::value_type;
  using category  = boost::iostreams::sink_tag;
  explicit container_sink(Container& container) : container_(container) {}
  std::streamsize write(const char_type* s, std::streamsize n) {
    container_.insert(container_.end(), s, s + n);
    return n;
  }
  Container& container() { return container_; }
  container_sink operator=(const container_sink&) = delete;

 private:
  Container& container_;
};

/**
 * @brief 从容器中读取和写入序列,
 * @warning 这个容器的迭代器必须支持随机访问, 是随机访问迭代器
 * @tparam Container 支持随机访问的容器
 */
template <typename Container>
class container_device {
 public:
  using char_type = typename Container::value_type;
  using category  = boost::iostreams::seekable_device_tag;
  explicit container_device(Container& container)
      : container_(container), pos_(0) {}

  std::streamsize read(char_type* s, std::streamsize n) {
    using namespace std;
    auto amt =
        static_cast<std::streamsize>(container_.size() - pos_);
    std::streamsize result = (min)(n, amt);
    if (result != 0) {
      std::copy(container_.begin() + pos_,
                container_.begin() + pos_ + result,
                s);
      pos_ += result;
      return result;
    } else {
      return -1;  // EOF
    }
  }
  std::streamsize write(const char_type* s, std::streamsize n) {
    using namespace std;
    std::streamsize result = 0;
    if (pos_ != container_.size()) {
      auto amt =
          static_cast<std::streamsize>(container_.size() - pos_);
      result = (min)(n, amt);
      std::copy(s, s + result, container_.begin() + pos_);
      pos_ += result;
    }
    if (result < n) {
      container_.insert(container_.end(), s, s + n);
      pos_ = container_.size();
    }
    return n;
  }
  boost::iostreams::stream_offset seek(boost::iostreams::stream_offset off, BOOST_IOS::seekdir way) {
    using namespace std;

    // Determine new value of pos_
    boost::iostreams::stream_offset next;
    if (way == BOOST_IOS::beg) {
      next = off;
    } else if (way == BOOST_IOS::cur) {
      next = pos_ + off;
    } else if (way == BOOST_IOS::end) {
      next = container_.size() + off - 1;
    } else {
      DOODLE_CHICK(false,doodle_error{"bad seek direction"});
      //      throw BOOST_IOSTREAMS_FAILURE("bad seek direction");
    }

    // Check for errors
    if (next < 0 || next > static_cast<boost::iostreams::stream_offset>(container_.size()))
      DOODLE_CHICK(false,doodle_error{"bad seek offset"});
    //      throw BOOST_IOSTREAMS_FAILURE("bad seek offset");

    pos_ = next;
    return pos_;
  }

  Container& container() { return container_; }
  container_device operator=(const container_device&) = delete;

 private:
  using size_type = typename Container::size_type;
  Container& container_;
  size_type pos_;
};

/**
 * @brief 读std::vector 流
 *
 * @tparam CharT char 类
 * @tparam Container 容器
 * @tparam Device 后端类
 * @tparam Buffer 缓冲区类
 */
template <class CharT     = char,
          class Container = std::vector<CharT>,
          class Device    = container_source<Container>,
          class Buffer    = boost::iostreams::stream_buffer<Device> >
class base_vector_istream : public std::istream {
  Buffer _vector_buffer;

 public:
  explicit base_vector_istream(Container& cnt)
      : std::istream(std::addressof(_vector_buffer)),
        _vector_buffer(Device(cnt)) {}
};

/**
 * @brief 写std::vector 流
 *
 * @tparam CharT char 类
 * @tparam Container 容器
 * @tparam Device 后端类
 * @tparam Buffer 缓冲区类
 */
template <class CharT     = char,
          class Container = std::vector<CharT>,
          class Device    = container_sink<Container>,
          class Buffer    = boost::iostreams::stream_buffer<Device> >
class base_vector_ostream : public std::ostream {
  Buffer _vector_buffer;

 public:
  explicit base_vector_ostream(Container& cnt)
      : _vector_buffer(Device(cnt)),
        std::ostream(std::addressof(_vector_buffer)) {}
};

/**
 * @brief 读写std::vector 流
 *
 * @tparam CharT char 类
 * @tparam Container 容器
 * @tparam Device 后端类
 * @tparam Buffer 缓冲区类
 */
template <class CharT     = char,
          class Container = std::vector<CharT>,
          class Device    = container_device<Container>,
          class Buffer    = boost::iostreams::stream_buffer<Device> >
class base_vector_iostream : public std::iostream {
  Buffer _vector_buffer;

 public:
  explicit base_vector_iostream(Container& cnt)
      : _vector_buffer(Device(cnt)),
        std::iostream(std::addressof(_vector_buffer)) {}
};
/**
 * @brief std char 容器
 *
 */
using vector_container = std::vector<char>;
/// std vector 读流
using vector_istream   = base_vector_istream<char>;
/// std vector 写流
using vector_ostream   = base_vector_ostream<char>;
/// std vector 读写流
using vector_iostream  = base_vector_iostream<char>;
}  // namespace doodle
