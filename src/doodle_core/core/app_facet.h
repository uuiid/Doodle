//
// Created by TD on 2022/9/30.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle {
namespace detail {
class DOODLE_CORE_API app_facet_interface {
 public:
  virtual ~app_facet_interface()                   = default;

  /**
   * 返回构面的名称以用来初始化命令行
   * @return 构面名称
   */
  [[nodiscard]] virtual const std::string& name() const noexcept = 0;

  /**
   * 初始化
   */
  virtual void operator()()                        = 0;

  /**
   * 结束清理
   */
  virtual void deconstruction()                    = 0;
};

}  // namespace detail
}  // namespace doodle
