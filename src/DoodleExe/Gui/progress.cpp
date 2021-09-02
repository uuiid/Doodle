//
// Created by TD on 2021/7/12.
//

#include "progress.h"

#include <DoodleLib/threadPool/long_term.h>

#include <nana/gui/msgbox.hpp>

namespace doodle {
progress::progress() = default;

progress::progress(nana::window in_w, long_term_ptr in_, std::string in_title)
    : nana::form(in_w, nana::size{600, 150}),
      p_layout(*this),
      _label(*this, "运行中..."),
      _pro(nana::progress(*this)),
      _text_box(*this) {
  p_layout.div(R"(<> 
  <
    weight=70%
    vertical 
    <>
    <
      weight=80%
      vertical
      pro
      arrange=[25,25,50]
      gap=5
    >
    <>
  > 
  <>
  )");
  p_layout.field("pro") << _label << _pro << _text_box;

  _pro.amount(1000);
  _sig_scoped_list.emplace_back(boost::signals2::scoped_connection{
      in_->sig_finished.connect([this, in_]() {
        _pro.value(1000);
        this->close();
      })});
  _sig_scoped_list.emplace_back(boost::signals2::scoped_connection{
      in_->sig_progress.connect([this, in_](std::double_t in_double) {
        auto k_v = in_->step(in_double) * 1000;
        _pro.value(((k_v < 0 ? 0 : k_v) > 999 ? 999 : k_v));
      })});
  _sig_scoped_list.emplace_back(boost::signals2::scoped_connection{
      in_->sig_message_result.connect(([this, in_](const std::string& in_str) {
        DOODLE_LOG_INFO(in_str);
        _text_box.caption(in_str);
        if (in_->fulfil()) {
          nana::msgbox msg{"结果"};
          msg << in_str;
          msg();
        }
      }))});
  p_layout.collocate();
  caption(in_title);
}

void progress::create_progress(nana::window in_w, const long_term_ptr& in_, const std::string& in_title) {
  progress k_{in_w, in_, in_title};

  if (in_->fulfil()) {
    DOODLE_LOG_INFO("已经完成， 不需要显示进度条")
    if (in_->message_result().empty()) {
      DOODLE_LOG_INFO("结果字符串为空, 直接返回")
      return;
    }
    nana::msgbox msg{in_w, "结果"};
    msg << in_->message_result();
    msg();
    return;
  }

  k_.modality();
}
progress::~progress() {
  _sig_scoped_list.clear();
}

}  // namespace doodle
