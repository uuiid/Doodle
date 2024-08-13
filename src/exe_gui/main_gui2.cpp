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
    w.run();
  } catch (...) {
    std::cerr << boost::current_exception_diagnostic_information() << std::endl;
    return 1;
  }

  return 0;
}
