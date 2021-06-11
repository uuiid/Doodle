#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <wx/wx.h>

namespace doodle {
class DOODLELIB_API ServerWidget : public wxFrame {
  wxSpinCtrl* p_sql_port;
  wxTextCtrl* p_sql_host;
  wxTextCtrl* p_sql_user;
  wxTextCtrl* p_sql_password;

  wxTextCtrl* p_cache_root;
  wxSpinCtrl* p_meta_rpc_port;
  wxSpinCtrl* p_file_rpc_port;

  wxButton* p_start_rpc;
  wxButton* p_reStart_rpc;

  RpcServerHandlePtr p_rpc_server_handle;
  void Init();
  void layoutServerWidget(wxSizer* layout);
  void bindServerWideget() const;

 public:
  explicit ServerWidget();

  DOODLE_DISABLE_COPY(ServerWidget)
};
}  // namespace doodle
