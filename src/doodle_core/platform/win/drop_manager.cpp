//
// Created by TD on 2021/12/31.
//

#include "drop_manager.h"

#include <doodle_core/core/core_sig.h>
#include <doodle_core/logger/logger.h>

#include <Windows.h>
#include <imgui.h>
#include <shellapi.h>
#include <tchar.h>

namespace doodle::win {

ULONG drop_manager::AddRef() { return InterlockedIncrement(&m_RefCount); }

ULONG drop_manager::Release() {
  auto nTemp = InterlockedDecrement(&m_RefCount);
  if (!nTemp) {
    begin_drop = false;
    delete this;
  }
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

STDMETHODIMP drop_manager::DragEnter(IDataObject *pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect) {
  DOODLE_LOG_INFO("开始 DragEnter");
  begin_drop     = true;

  // 使用 fmte
  FORMATETC fmte = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  STGMEDIUM stgm{};
  if (SUCCEEDED(pdto->GetData(&fmte, &stgm))) {
    auto hdrop     = reinterpret_cast<HDROP>(stgm.hGlobal);
    auto file_size = DragQueryFile(hdrop, 0xFFFFFFFF, nullptr, 0);
    std::vector<doodle::FSys::path> l_vector{};
    // 我们可以同时拖动多个文件，所以我们必须在这里循环
    for (UINT i = 0; i < file_size; i++) {
      std::size_t l_len = DragQueryFile(hdrop, i, nullptr, 0) + 1;
      std::unique_ptr<wchar_t[]> varbuf{new wchar_t[l_len]};

      UINT cch = DragQueryFile(hdrop, i, varbuf.get(), l_len);
      if (cch == 0) doodle::throw_exception(doodle::doodle_error{"拖拽文件获取失败"s});
      l_vector.emplace_back(varbuf.get());
    }
    DOODLE_LOG_INFO("查询到文件拖拽 :\n{}", fmt::join(l_vector, "\n"));
    ReleaseStgMedium(&stgm);
    drop_files = l_vector;
  }
  *pdwEffect &= DROPEFFECT_COPY;

  ImGuiIO &io = ImGui::GetIO();
  //  bool const want_absolute_pos = (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
  io.AddMousePosEvent(boost::numeric_cast<std::float_t>(ptl.x), boost::numeric_cast<std::float_t>(ptl.y));
  io.AddFocusEvent(true);

  return S_OK;
}

STDMETHODIMP drop_manager::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect) {
  *pdwEffect &= DROPEFFECT_COPY;

  begin_drop  = true;

  ImGuiIO &io = ImGui::GetIO();
  //  bool const want_absolute_pos = (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
  io.AddMousePosEvent(boost::numeric_cast<std::float_t>(ptl.x), boost::numeric_cast<std::float_t>(ptl.y));
  io.AddFocusEvent(true);

  return S_OK;
}

STDMETHODIMP drop_manager::DragLeave() {
  DOODLE_LOG_INFO("结束 DragLeave");
  drop_files.clear();

  ImGuiIO &io = ImGui::GetIO();
  //  bool const want_absolute_pos = (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
  io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
  io.AddFocusEvent(false);

  begin_drop = false;
  return S_OK;
}

STDMETHODIMP drop_manager::Drop(IDataObject *pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect) {
  DOODLE_LOG_INFO("放下 Drop");

  // 使用 fmte
  FORMATETC fmte = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  STGMEDIUM stgm{};

  if (SUCCEEDED(pdto->GetData(&fmte, &stgm))) {
    auto hdrop     = reinterpret_cast<HDROP>(stgm.hGlobal);
    auto file_size = DragQueryFile(hdrop, 0xFFFFFFFF, nullptr, 0);
    std::vector<doodle::FSys::path> l_vector{};
    // 我们可以同时拖动多个文件，所以我们必须在这里循环
    for (UINT i = 0; i < file_size; i++) {
      std::size_t l_len = DragQueryFile(hdrop, i, nullptr, 0) + 1;
      std::unique_ptr<wchar_t[]> varbuf{new wchar_t[l_len]};

      UINT cch = DragQueryFile(hdrop, i, varbuf.get(), l_len);
      if (cch == 0) doodle::throw_exception(doodle::doodle_error{"拖拽文件获取失败"s});
      l_vector.emplace_back(varbuf.get());
    }
    DOODLE_LOG_INFO("查询到文件拖拽 :\n{}", fmt::join(l_vector, "\n"));
    // 完成后我们必须释放数据
    ReleaseStgMedium(&stgm);

    std::swap(drop_files, l_vector);
  }

  // 为 ImGui 中的按钮 1 触发 MouseUp

  ImGuiIO &io = ImGui::GetIO();
  //  bool const want_absolute_pos = (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
  io.AddMousePosEvent(boost::numeric_cast<std::float_t>(ptl.x), boost::numeric_cast<std::float_t>(ptl.y));
  io.AddFocusEvent(true);

  // 以某种方式通知我们的应用程序我们已经完成了文件的拖动（以某种方式提供数据）
  begin_drop = false;
  *pdwEffect &= DROPEFFECT_COPY;
  return S_OK;
}
const std::vector<FSys::path> &drop_manager::GetDropFiles() const { return drop_files; }

ole_guard::ole_guard() {
  auto k_r = ::OleInitialize(nullptr);
  switch (k_r) {
    case S_OK:
      DOODLE_LOG_INFO("COM 库已在此线程上成功初始化");
      break;
    case S_FALSE:
      DOODLE_LOG_INFO("COM 库已在此线程上初始化");
      break;
    case RPC_E_CHANGED_MODE:
      throw_exception(doodle_error{
          "之前对CoInitializeEx的调用将此线程的并发模型指定为多线程单元 (MTA), "
          "这也可能表明发生了从中性线程单元到单线程单元的更改"});
      break;
    default:
      break;
  }
}
ole_guard::~ole_guard() { ::OleUninitialize(); }
}  // namespace doodle::win
