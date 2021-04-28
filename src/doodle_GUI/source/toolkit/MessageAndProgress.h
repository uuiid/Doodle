#pragma once

#include <doodle_GUI/doodle_global.h>
#include <wx/progdlg.h>

namespace doodle {
// BEGIN_DECLARE_EVENT_TYPES()
// DECLARE_EVENT_TYPE(DOODLE_THREAD, -101)
// END_DECLARE_EVENT_TYPES()
class MessageAndProgress {
  std::string p_message;
  wxProgressDialog* p_progress;
  wxMessageDialog* p_message_dialog;

  wxWindowIDRef p_mess_id;
  wxWindowIDRef p_prog_id;
  wxWindowIDRef p_t_id;

 public:
  MessageAndProgress(wxWindow* parent);

  template <typename T>
  void createProgress(std::shared_ptr<T> value);
};

template <typename T>
void MessageAndProgress::createProgress(std::shared_ptr<T> value) {
  this->p_progress->Show();
  //连接进度
  value->progress.connect([this](int i) {
    wxThreadEvent event{wxEVT_THREAD, p_t_id};
    event.SetInt(i);
    wxQueueEvent(this->p_progress, event.Clone());
  });
  //连接消息
  value->messagResult.connect([this](const std::string& message) {
    this->p_message.append(message);
  });
  //连接完成信号
  value->finished.connect([this] {
    wxThreadEvent event{wxEVT_THREAD, p_t_id};
    auto wxstr = wxString::FromUTF8(this->p_message);
    event.SetString(wxstr);
    wxQueueEvent(this->p_message_dialog, event.Clone());
  });
}
}  // namespace doodle