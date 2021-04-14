#include <doodle_GUI/source/mainWidght/mainWindows.h>
//logger是boost库使用者，放到qt上面能好点
#include <loggerlib/Logger.h>

#include <doodle_GUI/source/mainWidght/DragPushBUtton.h>
#include <doodle_GUI/source/mainWidght/systemTray.h>
#include <doodle_GUI/source/toolkit/MessageAndProgress.h>
#include <doodle_GUI/source/SettingWidght/SettingWidget.h>
#include <doodle_GUI/source/Metadata/View/ShotListView.h>

#include <boost/format.hpp>

#include <wx/gbsizer.h>
DOODLE_NAMESPACE_S

mainWindows::mainWindows()
    : wxFrame(nullptr, wxID_ANY, {"doodle"}) {
  SetMenuBar(new wxMenuBar{});
  CreateStatusBar(1);
  SetStatusText("doodle tools");

  auto k_parent  = new wxPanel{this, wxID_ANY};
  auto layout    = new wxBoxSizer{wxVERTICAL};
  auto k_butten  = new wxButton{k_parent, wxID_ANY, "test"};
  auto k_butten2 = new wxButton{k_parent, wxID_ANY, "test2"};
  auto k_butten3 = new wxButton{k_parent, wxID_ANY, "test3"};
  layout->Add(k_butten, /* wxGBPosition{0, 0}, wxGBSpan{1, 2}, */ wxSizerFlags{0}.Left().GetFlags());
  layout->Add(k_butten2, /* wxGBPosition{1, 1}, wxGBSpan{1, 1}, */ wxSizerFlags{0}.Right().GetFlags());
  layout->Add(k_butten3, /* wxGBPosition{1, 0}, wxGBSpan{1, 1}, */ wxSizerFlags{0}.Right().GetFlags());

  k_parent->SetSizer(layout);
  layout->SetSizeHints(this);

  k_butten->DragAcceptFiles(true);

  k_butten->Bind(
      wxEVT_BUTTON,
      [this](wxCommandEvent& event) {
        auto k_ = wxMessageDialog{this, "ok", "ok"};
        k_.ShowModal();
        std::cout << "ok" << std::endl;
      });
  k_butten->Bind(
      wxEVT_DROP_FILES,
      [this](wxDropFilesEvent& event) {
        if (event.GetNumberOfFiles() > 0) {
          auto k_   = wxMessageDialog{this, "file: "};
          auto k_s_ = event.GetFiles();
          k_.SetMessage(k_s_[0]);
          k_.ShowModal();
        }
      });

  // Make a menubar
  // wxMenu *file_menu = new wxMenu;

  // file_menu->Append(LAYOUT_TEST_PROPORTIONS, "&Proportions demo...\tF1");
  // file_menu->Append(LAYOUT_TEST_SIZER, "Test wx&FlexSizer...\tF2");
  // file_menu->Append(LAYOUT_TEST_NB_SIZER, "Test &notebook sizers...\tF3");
  // file_menu->Append(LAYOUT_TEST_GB_SIZER, "Test &gridbag sizer...\tF4");
  // file_menu->Append(LAYOUT_TEST_SET_MINIMAL, "Test Set&ItemMinSize...\tF5");
  // file_menu->Append(LAYOUT_TEST_NESTED, "Test nested sizer in a wxPanel...\tF6");
  // file_menu->Append(LAYOUT_TEST_WRAP, "Test wrap sizers...\tF7");

  // file_menu->AppendSeparator();
  // file_menu->Append(LAYOUT_QUIT, "E&xit", "Quit program");

  // wxMenu *help_menu = new wxMenu;
  // help_menu->Append(LAYOUT_ABOUT, "&About", "About layout demo...");

  // wxMenuBar *menu_bar = new wxMenuBar;

  // menu_bar->Append(file_menu, "&File");
  // menu_bar->Append(help_menu, "&Help");

  // // Associate the menu bar with the frame
  // SetMenuBar(menu_bar);

  // CreateStatusBar(2);
  // SetStatusText("wxWidgets layout demo");

  // wxPanel *p = new wxPanel(this, wxID_ANY);

  // we want to get a dialog that is stretchable because it
  // has a text ctrl in the middle. at the bottom, we have
  // two buttons which.

  // wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);

  // // 1) top: create wxStaticText with minimum size equal to its default size
  // topsizer->Add(
  //     new wxStaticText(p, wxID_ANY, "An explanation (wxALIGN_RIGHT)."),
  //     wxSizerFlags().Align(wxALIGN_RIGHT).Border(wxALL & ~wxBOTTOM, 5));
  // topsizer->Add(
  //     new wxStaticText(p, wxID_ANY, "An explanation (wxALIGN_LEFT)."),
  //     wxSizerFlags().Align(wxALIGN_LEFT).Border(wxALL & ~wxBOTTOM, 5));
  // topsizer->Add(
  //     new wxStaticText(p, wxID_ANY, "An explanation (wxALIGN_CENTRE_HORIZONTAL)."),
  //     wxSizerFlags().Align(wxALIGN_CENTRE_HORIZONTAL).Border(wxALL & ~wxBOTTOM, 5));

  // // 2) top: create wxTextCtrl with minimum size (100x60)
  // topsizer->Add(
  //     new wxTextCtrl(p, wxID_ANY, "My text (wxEXPAND).", wxDefaultPosition, wxSize(100, 60), wxTE_MULTILINE),
  //     wxSizerFlags(1).Expand().Border(wxALL, 5));

  // // 2.5) Gratuitous test of wxStaticBoxSizers
  // wxBoxSizer *statsizer = new wxStaticBoxSizer(
  //     new wxStaticBox(p, wxID_ANY, "A wxStaticBoxSizer"), wxVERTICAL);
  // statsizer->Add(
  //     new wxStaticText(p, wxID_ANY, "And some TEXT inside it"),
  //     wxSizerFlags().Border(wxALL, 30));
  // topsizer->Add(
  //     statsizer,
  //     wxSizerFlags(1).Expand().Border(wxALL, 10));

  // 2.7) And a test of wxGridSizer
  // wxGridSizer *gridsizer = new wxGridSizer(2, 5, 5);
  // gridsizer->Add(new wxStaticText(p, wxID_ANY, "Label"),
  //                wxSizerFlags().Align(wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL));
  // gridsizer->Add(new wxTextCtrl(p, wxID_ANY, "Grid sizer demo"),
  //                wxSizerFlags(1).Align(wxGROW | wxALIGN_CENTER_VERTICAL));
  // gridsizer->Add(new wxStaticText(p, wxID_ANY, "Another label"),
  //                wxSizerFlags().Align(wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL));
  // gridsizer->Add(new wxTextCtrl(p, wxID_ANY, "More text"),
  //                wxSizerFlags(1).Align(wxGROW | wxALIGN_CENTER_VERTICAL));
  // gridsizer->Add(new wxStaticText(p, wxID_ANY, "Final label"),
  //                wxSizerFlags().Align(wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL));
  // gridsizer->Add(new wxTextCtrl(p, wxID_ANY, "And yet more text"),
  //                wxSizerFlags().Align(wxGROW | wxALIGN_CENTER_VERTICAL));
  // topsizer->Add(
  //     gridsizer,
  //     wxSizerFlags().Proportion(1).Expand().Border(wxALL, 10));

  // #if wxUSE_STATLINE
  //   // 3) middle: create wxStaticLine with minimum size (3x3)
  //   topsizer->Add(
  //       new wxStaticLine(p, wxID_ANY, wxDefaultPosition, wxSize(3, 3), wxHORIZONTAL),
  //       wxSizerFlags().Expand());
  // #endif  // wxUSE_STATLINE

  // 4) bottom: create two centred wxButtons
  // wxBoxSizer *button_box = new wxBoxSizer(wxHORIZONTAL);
  // button_box->Add(
  //     new wxButton(p, wxID_ANY, "Two buttons in a box"),
  //     wxSizerFlags().Border(wxALL, 7));
  // button_box->Add(
  //     new wxButton(p, wxID_ANY, "(wxCENTER)"),
  //     wxSizerFlags().Border(wxALL, 7));

  // topsizer->Add(button_box, wxSizerFlags().Center());

  // p->SetSizer(topsizer);

  // don't allow frame to get smaller than what the sizers tell it and also set
  // the initial size as calculated by the sizers
  // topsizer->SetSizeHints(this);
}

Doodle::Doodle(){

};

bool Doodle::OnInit() {
  wxApp::OnInit();
  auto k_mainWindows = new mainWindows{};
  k_mainWindows->Show();
  return true;
}

DOODLE_NAMESPACE_E
