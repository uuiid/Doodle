#include <doodle_GUI/source/toolkit/MessageAndProgress.h>
#include <boost/locale.hpp>

namespace doodle {
// IMPLEMENT_DYNAMIC_CLASS();
// DEFINE_EVENT_TYPE(DOODLE_THREAD);
MessageAndProgress::MessageAndProgress(wxWindow* parent)
    : p_message(),
      p_progress(),
      p_message_dialog(),
      p_mess_id(wxWindow::NewControlId()),
      p_prog_id(wxWindow::NewControlId()),
      p_t_id(wxWindow::NewControlId()) {
  p_progress = new wxProgressDialog{
      wxString::FromUTF8("作业进度"),
      wxString::FromUTF8("进行中..."),
      100,
      parent,
      wxPD_AUTO_HIDE | wxPD_CAN_ABORT};
  p_progress->SetId(p_prog_id);
  p_message_dialog = new wxMessageDialog{parent, wxString::FromUTF8("完成作业")};
  p_message_dialog->SetId(p_mess_id);
  p_message_dialog->SetEvtHandlerEnabled(true);

  p_message_dialog->Bind(
      wxEVT_THREAD,
      [this](wxThreadEvent& event) {
        // p_progress->Destroy();
        //这个是刷新整个窗口的
        // wxWakeUpIdle();
        // auto str  = event.GetString();
        auto wstr = boost::locale::conv::utf_to_utf<wchar_t>(this->p_message);
        // auto bstr = boost::locale::conv::utf_to_utf<char>(wstr);
        // auto str_ = wxString::From8BitData(bstr.data(), bstr.length());
        auto str_ = wxString(wstr.data(), wstr.length());
        // auto str_ = wxString(bstr.data(), bstr.length());
        // auto str_ = wxString{wstr};
        p_message_dialog->SetMessage(str_);
        p_message_dialog->ShowModal();
      });
  p_progress->Bind(
      wxEVT_THREAD,
      [this](wxThreadEvent& event) {
        p_progress->Update(event.GetInt());
        event.Skip();
      });
}

}  // namespace doodle