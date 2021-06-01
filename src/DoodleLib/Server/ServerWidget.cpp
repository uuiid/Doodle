#include <DoodleLib/Server/ServerWidget.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/rpc/RpcServer.h>

#include <wx/spinctrl.h>

namespace doodle {
void ServerWidget::Init() {
  auto& set = CoreSet::getSet();
  p_sql_host->SetValue(ConvStr<wxString>(set.getSqlHost()));
  p_sql_port->SetValue(set.getSqlPort());
  p_sql_user->SetValue(ConvStr<wxString>(set.getSqlUser()));
  p_sql_password->SetValue(ConvStr<wxString>(set.getSqlPassword()));
  p_cache_root->SetValue(ConvStr<wxString>(set.getCacheRoot().generic_string()));
  p_file_rpc_port->SetValue(set.getFileRpcPort());
  p_meta_rpc_port->SetValue(set.getMetaRpcPort());
  p_start_rpc->SetLabel(ConvStr<wxString>("开始服务器"));
  p_reStart_rpc->SetLabel(ConvStr<wxString>("重启服务器"));
}

ServerWidget::ServerWidget()
    : wxFrame(nullptr, NewControlId(), ConvStr<wxString>("服务器设置")),
      p_sql_port(new wxSpinCtrl{this, NewControlId()}),
      p_sql_host(new wxTextCtrl{this, NewControlId()}),
      p_sql_user(new wxTextCtrl{this, NewControlId()}),
      p_sql_password(new wxTextCtrl{this, NewControlId()}),
      p_cache_root(new wxTextCtrl{this, NewControlId()}),
      p_meta_rpc_port(new wxSpinCtrl{this, NewControlId()}),
      p_file_rpc_port(new wxSpinCtrl{this, NewControlId()}),
      p_start_rpc(new wxButton{this, NewControlId()}),
      p_reStart_rpc(new wxButton{this, NewControlId()}) {
  auto layout = new wxFlexGridSizer{2};
  /// 布局函数
  layoutServerWidget(layout);
  /// 初始化值设置
  Init();
}
void ServerWidget::layoutServerWidget(wxFlexGridSizer* layout) {
  layout->Add(
      new wxStaticText{this, NewControlId(), ConvStr<wxString>("mysql ip地址")},
      wxSizerFlags{0}.Expand());
  layout->Add(p_sql_host, wxSizerFlags{0}.Expand());

  layout->Add(
      new wxStaticText{this, NewControlId(), ConvStr<wxString>("mysql 端口")},
      wxSizerFlags{0}.Expand());
  layout->Add(p_sql_port, wxSizerFlags{0}.Expand());

  layout->Add(
      new wxStaticText{this, NewControlId(), ConvStr<wxString>("mysql 用户名")},
      wxSizerFlags{0}.Expand());
  layout->Add(p_sql_user, wxSizerFlags{0}.Expand());

  layout->Add(
      new wxStaticText{this, NewControlId(), ConvStr<wxString>("mysql 密码")},
      wxSizerFlags{0}.Expand());
  layout->Add(p_sql_password, wxSizerFlags{0}.Expand());

  layout->Add(
      new wxStaticText{this, NewControlId(), ConvStr<wxString>("文件储存路径")},
      wxSizerFlags{0}.Expand());
  layout->Add(p_cache_root, wxSizerFlags{0}.Expand());

  layout->Add(
      new wxStaticText{this, NewControlId(), ConvStr<wxString>("元数据rpc服务端口")},
      wxSizerFlags{0}.Expand());
  layout->Add(p_meta_rpc_port, wxSizerFlags{0}.Expand());

  layout->Add(
      new wxStaticText{this, NewControlId(), ConvStr<wxString>("文件服务器rpc服务端口")},
      wxSizerFlags{0}.Expand());
  layout->Add(p_file_rpc_port, wxSizerFlags{0}.Expand());
}
void ServerWidget::bindServerWideget() const {
  auto& set = CoreSet::getSet();
  p_sql_host->Bind(wxEVT_TEXT, [&set](wxCommandEvent& in_event) {
    set.setSqlHost(ConvStr<std::string>(in_event.GetString()));
  });
  p_sql_port->Bind(wxEVT_SPINCTRL, [&set](wxCommandEvent& in_event) {
    set.setSqlPort(in_event.GetInt());
  });
  p_sql_user->Bind(wxEVT_TEXT, [&set](wxCommandEvent& in_event) {
    set.setSqlUser(ConvStr<std::string>(in_event.GetString()));
  });
  p_sql_password->Bind(wxEVT_TEXT, [&set](wxCommandEvent& in_event) {
    set.setSqlPassword(ConvStr<std::string>(in_event.GetString()));
  });
  p_cache_root->Bind(wxEVT_TEXT, [&set](wxCommandEvent& in_event) {
    set.setCacheRoot(ConvStr<std::string>(in_event.GetString()));
  });
  p_meta_rpc_port->Bind(wxEVT_SPINCTRL, [&set](wxCommandEvent& in_event) {
    set.setMetaRpcPort(in_event.GetInt());
  });
  p_file_rpc_port->Bind(wxEVT_SPINCTRL, [&set](wxCommandEvent& in_event) {
    set.setFileRpcPort(in_event.GetInt());
  });
  p_start_rpc->Bind(wxEVT_BUTTON, [&set](wxCommandEvent& in_evrnt) {
    RpcServer::runServer(set.getMetaRpcPort());
  });
  p_reStart_rpc->Bind(wxEVT_BUTTON, [&set](wxCommandEvent& in_event) {
    RpcServer::stop();
    RpcServer::runServer(set.getMetaRpcPort());
  });
}

}  // namespace doodle
