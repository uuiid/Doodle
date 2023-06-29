//
// Created by TD on 2021/12/31.
//

#include "doodle_core/platform/win/windows_alias.h"
#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/core/file_sys.h>

#include <bitset>
#include <memory>
#include <oleidl.h>
#include <unknwn.h>
#include <winrt/base.h>

namespace doodle::facet {
class gui_facet;
}
namespace doodle::win {
class DOODLE_CORE_API ole_guard {
 public:
  ole_guard();
  virtual ~ole_guard();
};

class DOODLE_CORE_API drop_manager : public winrt::implements<drop_manager, ::IDropTarget> {
 public:
 private:
  bool begin_drop_{};
  std::shared_ptr<std::vector<FSys::path>> drop_files;

 public:
  drop_manager() : begin_drop_(), drop_files(std::make_shared<std::vector<FSys::path>>()) {}

  // 当我们将文件拖入我们的应用程序视图时发生
  [[maybe_unused]] STDMETHODIMP DragEnter(IDataObject *pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect) override;
  // 当我们在携带文件的同时将鼠标拖到我们的应用程序视图上时发生（一直发生）
  [[maybe_unused]] STDMETHODIMP DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect) override;
  // 当我们从应用程序视图中拖出文件时发生
  [[maybe_unused]] STDMETHODIMP DragLeave() override;
  // 当我们释放鼠标按钮完成拖放操作时发生
  [[maybe_unused]] STDMETHODIMP Drop(IDataObject *pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect) override;

  void render();
};

class DOODLE_CORE_API drop_manager_guard {
 public:
  using drop_ptr_type = decltype(winrt::make_self<win::drop_manager>());
  explicit drop_manager_guard(drop_ptr_type &in_drop_ptr, wnd_handle in_hwnd);
  ~drop_manager_guard();

 private:
  drop_ptr_type drop_ptr_;
  wnd_handle hwnd_;
};

}  // namespace doodle::win
