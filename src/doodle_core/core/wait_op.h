//
// Created by TD on 2024/1/6.
//

#pragma once
#include <boost/system.hpp>

namespace doodle::detail {
struct wait_op {
 public:
  using func_type = void (*)(wait_op *op);

  void complete() {
    if (!handler_) return;
    auto handler = std::move(handler_);
    auto func    = func_;
    func(this);
    func_    = nullptr;
    handler_ = nullptr;
  }

  boost::system::error_code ec_{};

 protected:
  func_type func_{};               // The function to be called when the operation completes.
  std::shared_ptr<void> handler_;  // The handler to be called when the operation completes.

  wait_op(func_type func, std::shared_ptr<void> in_shared_ptr) : func_(func), handler_(std::move(in_shared_ptr)) {}
  ~wait_op() = default;
};
}  // namespace doodle::detail
