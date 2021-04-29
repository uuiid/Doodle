#include <doodle_GUI/source/toolkit/MessageAndProgress.h>
#include <boost/format.hpp>
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
        p_message_dialog->SetMessage(wxString::FromUTF8(this->p_message));
        p_message_dialog->ShowModal();
      });
  p_progress->Bind(
      wxEVT_THREAD,
      [this](wxThreadEvent& event) {
        // p_progress->set
        boost::format str_f{"进行中... %d"};
        auto k_i = (event.GetInt() > 100) ? 100 :event.GetInt();
        str_f % k_i;
        auto str = wxString::FromUTF8(str_f.str().append(" %"));
        p_progress->Update(k_i, str);
        event.Skip();
      });
}

}  // namespace doodle