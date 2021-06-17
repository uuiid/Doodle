#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <wx/wx.h>

namespace doodle {
class DOODLELIB_API ServerWidget : public wxFrame,public details::no_copy {
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
  void bindServerWidget() const;

 public:
  explicit ServerWidget();
};
}  // namespace doodle
