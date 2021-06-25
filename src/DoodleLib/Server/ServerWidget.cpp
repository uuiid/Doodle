#include <DoodleLib/DoodleApp.h>
#include <DoodleLib/Server/ServerWidget.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/rpc/RpcServerHandle.h>

#include <DoodleLib/core/Util.h>
#include <grpcpp/grpcpp.h>
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
      p_reStart_rpc(new wxButton{this, NewControlId()}),
      p_rpc_server_handle(std::make_shared<RpcServerHandle>()) {
  p_sql_port->SetMin(0);
  p_sql_port->SetMax(65535);

  p_meta_rpc_port->SetMin(0);
  p_meta_rpc_port->SetMax(65535);

  p_file_rpc_port->SetMin(0);
  p_file_rpc_port->SetMax(65535);

  auto layout = new wxBoxSizer{wxVERTICAL};
  /// 布局函数
  layoutServerWidget(layout);
  /// 初始化值设置
  Init();
  bindServerWidget();

  Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
//    auto k_r = wxMessageDialog{
//        this,
//        ConvStr<wxString>("关闭服务器并关闭程序")}
//                   .ShowModal();
//    if (k_r == wxID_OK) {
//      p_rpc_server_handle->stop();
//      wxGetApp().Exit();
//    } else {
//      if (event.CanVeto())
//        event.Veto(false);
//    }
  });
  p_rpc_server_handle->runServer(CoreSet::getSet().getMetaRpcPort(), CoreSet::getSet().getFileRpcPort());
}
void ServerWidget::layoutServerWidget(wxSizer* layout) {
  auto k_layout = wxUtil::labelAndWidget(this,"mysql ip地址: ", p_sql_host);
  layout->Add(k_layout, wxSizerFlags{0}.Expand().Proportion(1));
  k_layout = wxUtil::labelAndWidget(this,"mysql 端口: ", p_sql_port);
  layout->Add(k_layout, wxSizerFlags{0}.Expand().Proportion(1));

  k_layout = wxUtil::labelAndWidget(this,"mysql 用户名: ", p_sql_user);
  layout->Add(k_layout, wxSizerFlags{0}.Expand().Proportion(1));

  k_layout = wxUtil::labelAndWidget(this,"mysql 密码: ", p_sql_password);
  layout->Add(k_layout, wxSizerFlags{0}.Expand().Proportion(1));

  k_layout = wxUtil::labelAndWidget(this,"文件储存路径: ", p_cache_root);
  layout->Add(k_layout, wxSizerFlags{0}.Expand().Proportion(1));

  k_layout = wxUtil::labelAndWidget(this,"元数据rpc服务端口: ", p_meta_rpc_port);
  layout->Add(k_layout, wxSizerFlags{0}.Expand().Proportion(1));

  k_layout = wxUtil::labelAndWidget(this,"文件服务器rpc服务端口: ", p_file_rpc_port);
  layout->Add(k_layout, wxSizerFlags{0}.Expand().Proportion(1));

  auto h_layout = new wxBoxSizer{wxHORIZONTAL};
  h_layout->Add(p_start_rpc, wxSizerFlags{0}.Expand().Proportion(1));
  h_layout->Add(p_reStart_rpc, wxSizerFlags{0}.Expand().Proportion(1));

  layout->Add(h_layout, wxSizerFlags{0}.Expand().Proportion(1));

  SetSizer(layout);
  layout->SetSizeHints(this);
  this->Center();
}

void ServerWidget::bindServerWidget() const {
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
  p_start_rpc->Bind(wxEVT_BUTTON, [this, &set](wxCommandEvent& in_evrnt) {
    p_rpc_server_handle->runServer(set.getMetaRpcPort(), set.getFileRpcPort());
  });
  p_reStart_rpc->Bind(wxEVT_BUTTON, [&set, this](wxCommandEvent& in_event) {
    p_rpc_server_handle->stop();
    p_rpc_server_handle->runServer(set.getMetaRpcPort(), set.getFileRpcPort());
  });
}

}  // namespace doodle
