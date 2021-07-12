//
// Created by TD on 2021/7/12.
//

#include "progress.h"

#include <threadPool/long_term.h>

#include <nana/gui/msgbox.hpp>

namespace doodle {
progress::progress() = default;

progress::progress(nana::window in_w, long_term_ptr in_, std::string in_title)
    : nana::form(in_w),
      _pro(nana::progress(*this)) {
  _pro.amount(100);
  in_->sig_finished.connect([this]() {
    _pro.value(100);
  });
  in_->sig_progress.connect([this](int in_) {
    _pro.value(((in_ < 0 ? 0 : in_) > 99 ? 99 : in_));
  });
  in_->sig_message_result.connect(([this](const std::string& in_str) {
    nana::msgbox msg{*this, "结果"};
    msg << in_str;
    msg();
  }));

}

}  // namespace doodle