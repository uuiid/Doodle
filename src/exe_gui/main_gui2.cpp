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

using namespace Microsoft::WRL;

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

namespace doodle {

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
  WNDCLASSEX wcex;

  wcex.cbSize        = sizeof(WNDCLASSEX);
  wcex.style         = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc   = WndProc;
  wcex.cbClsExtra    = 0;
  wcex.cbWndExtra    = 0;
  wcex.hInstance     = hInstance;
  wcex.hIcon         = LoadIcon(hInstance, IDI_APPLICATION);
  wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName  = NULL;
  wcex.lpszClassName = szWindowClass;
  wcex.hIconSm       = LoadIcon(wcex.hInstance, IDI_APPLICATION);

  if (!RegisterClassEx(&wcex)) {
    MessageBox(NULL, _T("Call to RegisterClassEx failed!"), _T("Windows Desktop Guided Tour"), NULL);

    return 1;
  }

  // Store instance handle in our global variable
  hInst     = hInstance;

  // The parameters to CreateWindow explained:
  // szWindowClass: the name of the application
  // szTitle: the text that appears in the title bar
  // WS_OVERLAPPEDWINDOW: the type of window to create
  // CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
  // 500, 100: initial size (width, length)
  // NULL: the parent of this window
  // NULL: this application does not have a menu bar
  // hInstance: the first parameter from WinMain
  // NULL: not used in this application
  HWND hWnd = CreateWindow(
      szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1200, 900, NULL, NULL, hInstance, NULL
  );

  if (!hWnd) {
    MessageBox(NULL, _T("Call to CreateWindow failed!"), _T("Windows Desktop Guided Tour"), NULL);

    return 1;
  }

  // The parameters to ShowWindow explained:
  // hWnd: the value returned from CreateWindow
  // nCmdShow: the fourth parameter from WinMain
  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  // <-- WebView2 sample code starts here -->
  // Step 3 - Create a single WebView within the parent window
  // Locate the browser and set up the environment for WebView

  // CreateCoreWebView2EnvironmentWithOptions(
  //      nullptr, nullptr, nullptr,createEnvironmentCompletedHandler);
  auto l_createEnvironmentCompletedHandler =
      winrt::make<doodle::CoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(hWnd);
  CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr, l_createEnvironmentCompletedHandler.get());

  // <-- WebView2 sample code ends here -->

  // Main message loop:
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return (int)msg.wParam;
}

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  TCHAR greeting[] = _T("Hello, Windows desktop!");

  switch (message) {
    case WM_SIZE:
      if (webviewController != nullptr) {
        RECT bounds;
        GetClientRect(hWnd, &bounds);
        webviewController->put_Bounds(bounds);
      };
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
      break;
  }

  return 0;
}
