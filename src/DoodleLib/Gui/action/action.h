//
// Created by TD on 2021/6/17.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/threadPool/long_term.h>

#include <any>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/signals2.hpp>

namespace doodle {
namespace action_arg {
/**
 * @brief 这是一个动作参数对象， 使用时根据动作来确定参数
 *
 */
class DOODLELIB_API _arg {
 public:
  _arg()          = default;
  virtual ~_arg() = default;

  bool is_cancel = false;

  // _arg(_arg&& other) = default;
  // _arg && operator=(_arg&& other) = default;
};
class DOODLELIB_API arg_null : public _arg {
 public:
  arg_null() = default;
};
class DOODLELIB_API arg_int : public _arg {
 public:
  arg_int() = default;
  explicit arg_int(std::int32_t in_) : date(in_){};
  std::int32_t date{};
};
class DOODLELIB_API arg_str : public _arg {
 public:
  arg_str() = default;
  explicit arg_str(std::string in_) : date(std::move(in_)){};
  std::string date;
};
class DOODLELIB_API arg_path : public _arg {
 public:
  arg_path() = default;
  explicit arg_path(FSys::path in_) : date(std::move(in_)){};
  FSys::path date;
};
class DOODLELIB_API arg_paths : public _arg {
 public:
  arg_paths() = default;
  explicit arg_paths(std::vector<FSys::path> in_) : date(std::move(in_)){};
  std::vector<FSys::path> date;
};
}  // namespace action_arg
/**
 * @brief 动作，或者说是命令， 是围绕着数据的包装， 我们最终是需要运行的
 * 在创建动作时，需要喂入所需要的数据， 并在之后调用他
 * 或者我们喂入一些函子也是可以的
 */
class DOODLELIB_API action_base : public details::no_copy {
  friend DragFilesFactory;

 protected:
  std::string p_name;
  std::weak_ptr<long_term> p_term;
  std::mutex _mutex;

  void cancel(const std::string& in_str) {
    if (!p_term.expired()) {
      p_term.lock()->sig_finished();
      p_term.lock()->sig_message_result(in_str);
    }
  }

 public:
  action_base();
  /**
   * @brief 这个函数基本上可以作为显示为菜单的函数
   * @return 返回动作名称
   */
  virtual std::string class_name();

  /**
   * @brief 确认是否是异步动作
   *
   * @return true 是异步动作
   * @return false 不是异步动作
   */
  virtual bool is_async();
  /**
   * @brief 获得异步动作的信号属性(如果不是异步动作则为空指针)
   *
   * @return long_term_ptr 异步动作的信号类
   */
  [[nodiscard]] long_term_ptr get_long_term_signal() const;
  [[nodiscard]] long_term_ptr get_long_term_signal();
};

class DOODLELIB_API action : public action_base {
  friend DragFilesFactory;

 public:
  action();

  /**
   * @brief 运行动作， 异步时会直接返回进度信号类
   *
   * @param in_data 输入的数据
   * @param in_parent  输入数据的父类
   * @return long_term_ptr 返回的信号类可能为空
   */
  virtual long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) = 0;
  virtual long_term_ptr operator()(const MetadataPtr& in_data, const MetadataPtr& in_parent);
};

template <class arg_type>
class action_toolbox : public action {
 protected:
  arg_type p_date;
  virtual long_term_ptr run() = 0;
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override {
    return {};
  };

 public:
  action_toolbox()
      : action(),
        sig_get_arg(),
        p_date(){};

  long_term_ptr operator()(const MetadataPtr& in_data, const MetadataPtr& in_parent) override {
    return run();
  };
  boost::signals2::signal<arg_type()> sig_get_arg;
};
/**
 * 动作的中间对象,用来实现转换回调函数
 * @tparam arg_type 需要从gui获得的数据
 */
template <class arg_type>
class DOODLELIB_API action_indirect : public action {
 protected:
  arg_type _arg_type;

 public:
  action_indirect() = default;

  virtual bool is_accept(const arg_type& in_any) { return false; };
  boost::signals2::signal<arg_type()> sig_get_arg;
};

template <class arg_type>
class actn_composited : public action_indirect<arg_type> {
 protected:

 public:
  using arg_ = arg_type;
  actn_composited() = default;

  void set_class_name(const std::string& in_name);

};

class DOODLELIB_API actn_null : public action_indirect<action_arg::arg_null> {
 public:
  actn_null();

  inline long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override { return {}; };
};
}  // namespace doodle

/// 内部实现
namespace doodle {
template <class arg_type>
void actn_composited<arg_type>::set_class_name(const std::string& in_name) {
  action::p_name = in_name;
}
}  // namespace doodle
