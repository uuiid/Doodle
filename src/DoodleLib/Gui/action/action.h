//
// Created by TD on 2021/6/17.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

#include <any>
#include <boost/signals2.hpp>

namespace doodle {

/**
 * @brief 动作，或者说是命令， 是围绕着数据的包装， 我们最终是需要运行的
 * 在创建动作时，需要喂入所需要的数据， 并在之后调用他
 * 或者我们喂入一些函子也是可以的
 */

class DOODLELIB_API action : public details::no_copy {
  friend DragFilesFactory;

 protected:
  /**
   * @brief 这个是喂入的数据
   */
  std::any p_any;
  std::string p_name;

 public:
  /**
   * @brief 这是一个动作参数对象， 使用时根据动作来确定参数
   * 
   */
  class DOODLELIB_API _arg : public details::no_copy {
   public:
    _arg()          = default;
    virtual ~_arg() = default;
  };
  class DOODLELIB_API arg_int : public _arg {
   public:
    std::int32_t date;
  };

  class DOODLELIB_API arg_str : public _arg {
   public:
    std::string date;
  };

  action();
  explicit action(std::any&& in_any);

  virtual bool is_accept(const _arg& in_any);

  /**
   * @brief 这里是喂入函子的地方， 我们使用信号插槽模式
   *
   * 在这里我们要注意信号插槽模式的优先级别高于构造函数中的any值，
   * @warning 在同时使用两张的情况下优先信号槽
   */
  boost::signals2::signal<std::any()> sig_get_input;
  boost::signals2::signal<_arg()> sig_get_arg;
  /**
   * @brief 设置值
   * @param in_any 要设置的数据
   */
  void set_any(std::any&& in_any);
  /**
   * @brief 设置值
   * @warning 如果使用线程不安全的gui, 注意在gui线程调用这个函数, 这个函数使用信号获得any
   */
  void set_any();
  /**
   * @brief 这个函数基本上可以作为显示为菜单的函数
   * @return 返回动作名称
   */
  virtual std::string class_name();

  virtual void run(const MetadataPtr& in_data) = 0;
  virtual void operator()(const MetadataPtr& in_data);
};

}  // namespace doodle
