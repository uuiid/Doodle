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

class DOODLELIB_API Action : public details::no_copy {
  friend DragFilesFactory;

 protected:
  /**
   * @brief这个是喂入的数据
   */
  std::any p_any;
  std::string p_name;
 public:
  Action();
  explicit Action(std::any&& in_any);

  /**
   * @brief 这里是喂入函子的地方， 我们使用信号插槽模式
   *
   * 在这里我们要注意信号插槽模式的优先级别高于构造函数中的any值，
   * @warning 在同时使用两张的情况下优先信号槽
   */
  boost::signals2::signal<std::any()> sig_get_input;
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
   * 这个函数基本上可以作为显示为菜单的函数
   * @return
   */
  virtual std::string class_name() = 0;

  virtual void run(const MetadataPtr& in_data)        = 0;
  virtual void operator()(const MetadataPtr& in_data) = 0;
};

}  // namespace doodle
