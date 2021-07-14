//
// Created by TD on 2021/7/12.
//

#include "progress.h"

#include <threadPool/long_term.h>

#include <nana/gui/msgbox.hpp>

namespace doodle {
progress::progress() = default;

progress::progress(nana::window in_w, long_term_ptr in_, std::string in_title)
    : nana::form(in_w, nana::size{600, 150}),
      p_layout(*this),
      _label(*this, "运行中..."),
      _pro(nana::progress(*this)) {
  p_layout.div(R"(<> 
  <
    weight=70%
    vertical 
    <>
    <
      weight=80%
      vertical
      pro
      arrange=[25,25]
    >
    <>
  > 
  <>
  )");
  p_layout.field("pro") << _label << _pro;
  _pro.amount(100);
  in_->sig_finished.connect([this]() {
    _pro.value(100);
    this->close();
  });
  in_->sig_progress.connect([this, in_](std::double_t in_double) {
    auto k_v = in_->step(in_double);
    _pro.value(((k_v < 0 ? 0 : k_v) > 99 ? 99 : k_v));
  });
  in_->sig_message_result.connect(([this](const std::string& in_str) {
    nana::msgbox msg{*this, "结果"};
    msg << in_str;
    msg();
  }));
  p_layout.collocate();
}

void progress::create_progress(nana::window in_w, long_term_ptr in_, std::string in_title) {
  if (in_->fulfil()) {
    DOODLE_LOG_INFO("已经完成， 不需要显示进度条")
    nana::msgbox msg{in_w, "结果"};
    msg << in_->message_result();
    msg();
    return;
  }

  progress k_{in_w, in_, in_title};
  k_.modality();
}

}  // namespace doodle
