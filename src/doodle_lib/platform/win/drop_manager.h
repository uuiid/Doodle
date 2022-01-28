//
// Created by TD on 2021/12/31.
//

#include <doodle_lib/doodle_lib_fwd.h>

#include <oleidl.h>

void OpenFilesFromDataObject(IDataObject *pdto);
namespace doodle::win {

class DOODLELIB_API ole_guard {
 public:
  ole_guard();
  ~ole_guard();
};

class DOODLELIB_API drop_manager : public IDropTarget {
 private:
  LONG m_RefCount;

 public:
  drop_manager() : m_RefCount(0){};
  STDMETHODIMP_(ULONG)
  AddRef() override;

  STDMETHODIMP_(ULONG)
  Release() override;
  //  ~drop_manager();

  STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override;

  STDMETHODIMP DragEnter(IDataObject *pdto,
                         DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect) override;
  STDMETHODIMP DragOver(DWORD grfKeyState,
                        POINTL ptl, DWORD *pdwEffect) override;
  STDMETHODIMP DragLeave() override;
  STDMETHODIMP Drop(IDataObject *pdto, DWORD grfKeyState,
                    POINTL ptl, DWORD *pdwEffect) override;
};

}  // namespace doodle::win
