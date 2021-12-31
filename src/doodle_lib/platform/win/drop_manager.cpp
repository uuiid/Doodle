//
// Created by TD on 2021/12/31.
//

#include "drop_manager.h"

#include <tchar.h>
#include <Windows.h>
#include <shellapi.h>

namespace doodle::win {

ULONG drop_manager::AddRef() {
  return InterlockedIncrement(&m_RefCount);
}

ULONG drop_manager::Release() {
  auto nTemp = InterlockedDecrement(&m_RefCount);
  if (!nTemp) delete this;
  return nTemp;
}
STDMETHODIMP drop_manager::QueryInterface(const IID &riid, void **ppv) {
  if (riid == IID_IUnknown || riid == IID_IDropTarget) {
    *ppv = this;
    AddRef();
    return S_OK;
  }
  *ppv = nullptr;
  return E_NOINTERFACE;
}
//当我们将文件拖入我们的应用程序视图时发生
STDMETHODIMP drop_manager::DragEnter(IDataObject *pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect) {
  DOODLE_LOG_INFO("开始 DragEnter");

  *pdwEffect &= DROPEFFECT_COPY;
  return S_OK;
}
//当我们在携带文件的同时将鼠标拖到我们的应用程序视图上时发生（回车后，离开前）
STDMETHODIMP drop_manager::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect) {
  DOODLE_LOG_INFO("开始 DragOver");
  *pdwEffect &= DROPEFFECT_COPY;
  return S_OK;
}
//当我们从应用程序视图中拖出文件时发生
STDMETHODIMP drop_manager::DragLeave() {
  DOODLE_LOG_INFO("开始 DragLeave");
  return S_OK;
}
//当我们释放鼠标按钮完成拖放操作时发生
STDMETHODIMP drop_manager::Drop(IDataObject *pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect) {
  DOODLE_LOG_INFO("开始 Drop");

  //使用 fmte
  FORMATETC fmte = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  STGMEDIUM stgm{};

  if (SUCCEEDED(pdto->GetData(&fmte, &stgm))) {
    auto hdrop     = reinterpret_cast<HDROP>(stgm.hGlobal);
    auto file_size = DragQueryFile(hdrop, 0xFFFFFFFF, nullptr, 0);
    std::vector<doodle::FSys::path> l_vector{};
    //我们可以同时拖动多个文件，所以我们必须在这里循环
    for (UINT i = 0; i < file_size; i++) {
      std::size_t l_len = DragQueryFile(hdrop, i, nullptr, 0) + 1;
      std::unique_ptr<wchar_t[]> varbuf{new wchar_t[l_len]};

      UINT cch = DragQueryFile(hdrop, i, varbuf.get(), l_len);
      doodle::chick_true<doodle::doodle_error>(cch != 0, DOODLE_LOC, "拖拽文件获取失败");
      l_vector.emplace_back(varbuf.get());
    }
    DOODLE_LOG_INFO("查询到文件拖拽 :\n{}", fmt::join(l_vector, "\n"));
    // 完成后我们必须释放数据
    ReleaseStgMedium(&stgm);

    //以某种方式通知我们的应用程序我们已经完成了文件的拖动（以某种方式提供数据）
  }

  //为 ImGui 中的按钮 1 触发 MouseUp

  *pdwEffect &= DROPEFFECT_COPY;
  return S_OK;
}
}  // namespace doodle::win
