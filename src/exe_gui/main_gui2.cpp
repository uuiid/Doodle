//
// Created by TD on 24-8-1.
//

// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c

#include <nlohmann/json.hpp>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <wil/com.h>
#include <wil/cppwinrt.h>
#include <windows.h>
#include <winrt/base.h>
// #include <wrl.h>
// <IncludeHeader>
// include WebView2 header
#include "doodle_lib/doodle_lib_fwd.h"

#include "WebView2.h"
// </IncludeHeader>
#include <webview.h>

namespace doodle {
// Global variables

// The main window class name.
static TCHAR szWindowClass[] = _T("DoodleDesktopApp");

// The string that appears in the application's title bar.
static TCHAR szTitle[]       = _T("Doodle Web");

HINSTANCE hInst;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Pointer to WebViewController
static wil::com_ptr<ICoreWebView2Controller> webviewController;

// Pointer to WebView window
static wil::com_ptr<ICoreWebView2> webview;

class FilePath_CoreWebView2WebMessageReceivedEventHandler
    : public winrt::implements<
          FilePath_CoreWebView2WebMessageReceivedEventHandler, ::ICoreWebView2WebMessageReceivedEventHandler> {
  wil::com_ptr<ICoreWebView2> webview_view2_;

 public:
  FilePath_CoreWebView2WebMessageReceivedEventHandler() = default;
  explicit FilePath_CoreWebView2WebMessageReceivedEventHandler(ICoreWebView2* in_view2) : webview_view2_(in_view2) {}

  HRESULT STDMETHODCALLTYPE Invoke(
      /* [in] */ ICoreWebView2* sender,
      /* [in] */ ICoreWebView2WebMessageReceivedEventArgs* args
  ) override {
    wil::com_ptr<ICoreWebView2WebMessageReceivedEventArgs2> args2 =
        wil::com_ptr<ICoreWebView2WebMessageReceivedEventArgs>(args).query<ICoreWebView2WebMessageReceivedEventArgs2>();
    wil::com_ptr<ICoreWebView2ObjectCollectionView> objectsCollection;
    args2->get_AdditionalObjects(&objectsCollection);
    unsigned int length;
    objectsCollection->get_Count(&length);

    // Array of file paths to be sent back to the webview as JSON
    std::vector<FSys::path> l_files{};
    for (unsigned int i = 0; i < length; i++) {
      wil::com_ptr<IUnknown> object;
      objectsCollection->GetValueAtIndex(i, &object);
      if (wil::com_ptr<ICoreWebView2File> file = object.query<ICoreWebView2File>(); file) {
        // Add the file to message to be sent back to webview
        wil::unique_cotaskmem_string path;
        file->get_Path(&path);
        l_files.emplace_back(path.get());
      }
    }
    return webview_view2_->PostWebMessageAsJson(conv::utf_to_utf<wchar_t>(nlohmann::json{l_files}.dump()).c_str());
  }
};

class CoreWebView2CreateCoreWebView2ControllerCompletedHandler
    : public winrt::implements<
          CoreWebView2CreateCoreWebView2ControllerCompletedHandler,
          ::ICoreWebView2CreateCoreWebView2ControllerCompletedHandler> {
  HWND hwnd_;

 public:
  CoreWebView2CreateCoreWebView2ControllerCompletedHandler() = default;
  explicit CoreWebView2CreateCoreWebView2ControllerCompletedHandler(HWND hwnd) : hwnd_(hwnd) {}
  HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller* controller) override {
    if (controller != nullptr) {
      webviewController = controller;
      controller->get_CoreWebView2(&webview);
    }

    // Add a few settings for the webview
    // The demo step is redundant since the values are the default settings
    wil::com_ptr<ICoreWebView2Settings> settings;
    webview->get_Settings(&settings);
    settings->put_IsScriptEnabled(TRUE);
    settings->put_AreDefaultScriptDialogsEnabled(TRUE);
    settings->put_IsWebMessageEnabled(TRUE);

    // Resize WebView to fit the bounds of the parent window
    RECT bounds;
    GetClientRect(hwnd_, &bounds);
    controller->put_Bounds(bounds);

    // Schedule an async task to navigate to Bing
    webview->Navigate(L"http://127.0.0.1:5173/");

    // <NavigationEvents>
    // Step 4 - Navigation events
    // register an ICoreWebView2NavigationStartingEventHandler to cancel any non-https navigation
    EventRegistrationToken token;
    webview->add_WebMessageReceived(
        winrt::make<FilePath_CoreWebView2WebMessageReceivedEventHandler>(webview.get()).get(), &token
    );
    // webview->add_NavigationStarting(
    //     Callback<ICoreWebView2NavigationStartingEventHandler>(
    //         [](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
    //           wil::unique_cotaskmem_string uri;
    //           args->get_Uri(&uri);
    //           std::wstring source(uri.get());
    //           if (source.substr(0, 5) != L"https") {
    //             args->put_Cancel(true);
    //           }
    //           return S_OK;
    //         }
    //     ).Get(),
    //     &token
    // );
    // </NavigationEvents>

    // <Scripting>
    // Step 5 - Scripting
    // Schedule an async task to add initialization script that freezes the Object object
    webview->AddScriptToExecuteOnDocumentCreated(L"Object.freeze(Object);", nullptr);
    // Schedule an async task to get the document URL
    // webview->ExecuteScript(
    //     L"window.document.URL;", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
    //                                  [](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
    //                                    LPCWSTR URL = resultObjectAsJson;
    //                                    // doSomethingWithURL(URL);
    //                                    return S_OK;
    //                                  }
    //                              ).Get()
    // );
    // </Scripting>

    // <CommunicationHostWeb>
    // Step 6 - Communication between host and web content
    // Set an event handler for the host to return received message back to the web content
    // webview->add_WebMessageReceived(
    //     Callback<ICoreWebView2WebMessageReceivedEventHandler>(
    //         [](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
    //           wil::unique_cotaskmem_string message;
    //           args->TryGetWebMessageAsString(&message);
    //           // processMessage(&message);
    //           webview->PostWebMessageAsString(message.get());
    //           return S_OK;
    //         }
    //     ).Get(),
    //     &token
    // );

    // Schedule an async task to add initialization script that
    // 1) Add an listener to print message from the host
    // 2) Post document URL to the host
    webview->AddScriptToExecuteOnDocumentCreated(
        L"window.chrome.webview.addEventListener(\'message\', event => alert(event.data));"
        L"window.chrome.webview.postMessage(window.document.URL);",
        nullptr
    );
    // </CommunicationHostWeb>

    return S_OK;
  }
};

class CoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
    : public winrt::implements<
          CoreWebView2CreateCoreWebView2EnvironmentCompletedHandler,
          ::ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler> {
  HWND hwnd_;

 public:
  CoreWebView2CreateCoreWebView2EnvironmentCompletedHandler() = default;
  explicit CoreWebView2CreateCoreWebView2EnvironmentCompletedHandler(HWND hwnd) : hwnd_(hwnd) {}
  HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment* environment) override {
    if (FAILED(result)) {
      return result;
    }
    auto l_controller = winrt::make<CoreWebView2CreateCoreWebView2ControllerCompletedHandler>();
    return environment->CreateCoreWebView2Controller(hwnd_, l_controller.get());
  }
};
}  // namespace doodle

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
  try {
    long count = 0;

    webview::webview w(true, nullptr);
    w.set_title("doodle web");
    w.set_size(1200, 900, WS_OVERLAPPEDWINDOW);

    // A binding that counts up or down and immediately returns the new value.
    w.bind("get_file_path", [&](const std::string& req) -> std::string {
      std::cout << req << std::endl;

      return "test";
    });
    w.navigate("http://127.0.0.1:5173/");
    w.dispatch();

    w.run();
  } catch (...) {
    std::cerr << boost::current_exception_diagnostic_information() << std::endl;
    return 1;
  }

  return 0;
}
