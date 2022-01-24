//
// Created by TD on 2022/1/21.
//

#include "image_loader.h"
#include <opencv2/opencv.hpp>
#include <metadata/project.h>
#include <metadata/image_icon.h>
#include <app/app.h>
#include <platform/win/wnd_proc.h>

#include <DirectXTK/ScreenGrab.h>
#include <wincodec.h>

//#include <winrt/base.h>
#include <atlbase.h>
#include <atlwin.h>
/// \brief 显卡驱动导入
#include <d3d11.h>
namespace doodle {
namespace {
template <class T>
struct win_ptr_delete {
  void operator()(T* ptr) const {
    if (ptr)
      ptr->Release();
  }
};
template <class T>
struct guard_win_ptr_delete {
  T* p_t;
  explicit guard_win_ptr_delete(T* in) : p_t(in) {}
  ~guard_win_ptr_delete() {
    if (p_t) p_t->Release();
  }
};

#if 0
class screenshot_win {
 public:
  HRESULT CreateDirect3DDevice(IDXGIAdapter1* g) {
    HRESULT hr = S_OK;

    // Driver types supported
    D3D_DRIVER_TYPE DriverTypes[] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_REFERENCE,
        };
    UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

    // Feature levels supported
    D3D_FEATURE_LEVEL FeatureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1};
    UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

    D3D_FEATURE_LEVEL FeatureLevel;

    // Create device
    for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex) {
      hr = D3D11CreateDevice(g, DriverTypes[DriverTypeIndex], nullptr, D3D11_CREATE_DEVICE_VIDEO_SUPPORT, FeatureLevels, NumFeatureLevels,
                             D3D11_SDK_VERSION, &device, &FeatureLevel, &context);
      if (SUCCEEDED(hr)) {
        // Device creation success, no need to loop anymore
        break;
      }
    }
    if (FAILED(hr))
      return hr;

    return S_OK;
  }

  std::vector<BYTE> buf;
  CComPtr<ID3D11Device> device;
  CComPtr<ID3D11DeviceContext> context;
  CComPtr<IDXGIOutputDuplication> lDeskDupl;
  CComPtr<ID3D11Texture2D> lGDIImage;
  CComPtr<ID3D11Texture2D> lDestImage;
  DXGI_OUTDUPL_DESC lOutputDuplDesc = {};

  static void GetAdapters(std::vector<CComPtr<IDXGIAdapter1>>& a) {
    CComPtr<IDXGIFactory1> df;
    CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&df);
    a.clear();
    if (!df)
      return;
    int L = 0;
    for (;;) {
      CComPtr<IDXGIAdapter1> lDxgiAdapter;
      df->EnumAdapters1(L, &lDxgiAdapter);
      if (!lDxgiAdapter)
        break;
      L++;
      a.push_back(lDxgiAdapter);
    }
    return;
  }

  bool Get(IDXGIResource* lDesktopResource, bool Curs, RECT* rcx = 0) {
    // QI for ID3D11Texture2D
    CComPtr<ID3D11Texture2D> lAcquiredDesktopImage;
    if (!lDesktopResource)
      return 0;
    auto hr = lDesktopResource->QueryInterface(IID_PPV_ARGS(&lAcquiredDesktopImage));
    if (!lAcquiredDesktopImage)
      return 0;
    lDesktopResource = 0;

    // Copy image into GDI drawing texture
    context->CopyResource(lGDIImage, lAcquiredDesktopImage);

    // Draw cursor image into GDI drawing texture
    CComPtr<IDXGISurface1> lIDXGISurface1;

    lIDXGISurface1 = lGDIImage;

    if (!lIDXGISurface1)
      return 0;

    CURSORINFO lCursorInfo = {0};
    lCursorInfo.cbSize     = sizeof(lCursorInfo);
    auto lBoolres          = GetCursorInfo(&lCursorInfo);
    if (lBoolres == TRUE) {
      if (lCursorInfo.flags == CURSOR_SHOWING && Curs) {
        auto lCursorPosition = lCursorInfo.ptScreenPos;
        //                auto lCursorSize = lCursorInfo.cbSize;
        HDC lHDC;
        lIDXGISurface1->GetDC(FALSE, &lHDC);
        DrawIconEx(
            lHDC,
            lCursorPosition.x,
            lCursorPosition.y,
            lCursorInfo.hCursor,
            0,
            0,
            0,
            0,
            DI_NORMAL | DI_DEFAULTSIZE);
        lIDXGISurface1->ReleaseDC(nullptr);
      }
    }

    // Copy image into CPU access texture
    context->CopyResource(lDestImage, lGDIImage);

    // Copy from CPU access texture to bitmap buffer
    D3D11_MAPPED_SUBRESOURCE resource;
    UINT subresource = D3D11CalcSubresource(0, 0, 0);
    hr               = context->Map(lDestImage, subresource, D3D11_MAP_READ_WRITE, 0, &resource);
    if (FAILED(hr))
      return 0;

    auto sz  = lOutputDuplDesc.ModeDesc.Width * lOutputDuplDesc.ModeDesc.Height * 4;
    auto sz2 = sz;
    buf.resize(sz);
    if (rcx) {
      sz2 = (rcx->right - rcx->left) * (rcx->bottom - rcx->top) * 4;
      buf.resize(sz2);
      sz = sz2;
    }

    UINT lBmpRowPitch = lOutputDuplDesc.ModeDesc.Width * 4;
    if (rcx)
      lBmpRowPitch = (rcx->right - rcx->left) * 4;
    UINT lRowPitch = std::min<UINT>(lBmpRowPitch, resource.RowPitch);

    BYTE* sptr     = reinterpret_cast<BYTE*>(resource.pData);
    BYTE* dptr     = buf.data() + sz - lBmpRowPitch;
    if (rcx)
      sptr += rcx->left * 4;
    for (size_t h = 0; h < lOutputDuplDesc.ModeDesc.Height; ++h) {
      if (rcx && h < (size_t)rcx->top) {
        sptr += resource.RowPitch;
        continue;
      }
      if (rcx && h >= (size_t)rcx->bottom)
        break;
      memcpy_s(dptr, lBmpRowPitch, sptr, lRowPitch);
      sptr += resource.RowPitch;
      dptr -= lBmpRowPitch;
    }
    context->Unmap(lDestImage, subresource);
    return 1;
  }

  bool Prepare(UINT Output = 0) {
    // Get DXGI device
    CComPtr<IDXGIDevice> lDxgiDevice;
    lDxgiDevice = device;
    if (!lDxgiDevice)
      return 0;

    // Get DXGI adapter
    HRESULT hr = 0;

    CComPtr<IDXGIAdapter> lDxgiAdapter;
    hr = lDxgiDevice->GetParent(
        __uuidof(IDXGIAdapter),
        reinterpret_cast<void**>(&lDxgiAdapter));

    if (FAILED(hr))
      return 0;

    lDxgiDevice = 0;

    // Get output
    CComPtr<IDXGIOutput> lDxgiOutput;
    hr = lDxgiAdapter->EnumOutputs(Output, &lDxgiOutput);
    if (FAILED(hr))
      return 0;

    lDxgiAdapter = 0;

    DXGI_OUTPUT_DESC lOutputDesc;
    hr = lDxgiOutput->GetDesc(&lOutputDesc);

    // QI for Output 1
    CComPtr<IDXGIOutput1> lDxgiOutput1;
    lDxgiOutput1 = lDxgiOutput;
    if (!lDxgiOutput1)
      return 0;

    lDxgiOutput = 0;

    // Create desktop duplication
    hr          = lDxgiOutput1->DuplicateOutput(
                 device,
                 &lDeskDupl);

    if (FAILED(hr))
      return 0;

    lDxgiOutput1 = 0;

    // Create GUI drawing texture
    lDeskDupl->GetDesc(&lOutputDuplDesc);
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width                = lOutputDuplDesc.ModeDesc.Width;
    desc.Height               = lOutputDuplDesc.ModeDesc.Height;
    desc.Format               = lOutputDuplDesc.ModeDesc.Format;
    desc.ArraySize            = 1;
    desc.BindFlags            = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags            = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
    desc.SampleDesc.Count     = 1;
    desc.SampleDesc.Quality   = 0;
    desc.MipLevels            = 1;
    desc.CPUAccessFlags       = 0;
    desc.Usage                = D3D11_USAGE_DEFAULT;
    lGDIImage                 = 0;
    hr                        = device->CreateTexture2D(&desc, NULL, &lGDIImage);
    if (FAILED(hr))
      return 0;

    if (lGDIImage == nullptr)
      return 0;

    // Create CPU access texture
    desc.Width              = lOutputDuplDesc.ModeDesc.Width;
    desc.Height             = lOutputDuplDesc.ModeDesc.Height;
    desc.Format             = lOutputDuplDesc.ModeDesc.Format;
    desc.ArraySize          = 1;
    desc.BindFlags          = 0;
    desc.MiscFlags          = 0;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.MipLevels          = 1;
    desc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    desc.Usage              = D3D11_USAGE_STAGING;
    lDestImage              = 0;
    hr                      = device->CreateTexture2D(&desc, NULL, &lDestImage);
    if (FAILED(hr))
      return 0;

    if (lDestImage == nullptr)
      return 0;

    return 1;
  }
};
#endif

}  // namespace

class image_loader::impl {
 public:
};

image_loader::image_loader()
    : p_i() {
}
bool image_loader::load(const entt::handle& in_handle) {
  chick_true<doodle_error>(in_handle.any_of<image_icon>(), DOODLE_LOC, "缺失图标组件");
  auto k_reg = g_reg();
  chick_true<doodle_error>(k_reg->try_ctx<project>(), DOODLE_LOC, "缺失项目上下文");

  auto l_local_path = k_reg->ctx<project>().p_path / in_handle.get<image_icon>().path;

  auto k_image      = cv::imread(l_local_path.generic_string());
  chick_true<doodle_error>(!k_image.empty(), DOODLE_LOC, "open cv not read image");

  // 获得全局GPU渲染对象
  auto k_g = app::Get().d3dDevice;

  /// \brief 转换图像
  cv::cvtColor(k_image, k_image, cv::COLOR_BGR2RGBA);

  D3D11_TEXTURE2D_DESC k_tex_desc{};
  k_tex_desc.Width            = k_image.cols;
  k_tex_desc.Height           = k_image.rows;
  k_tex_desc.MipLevels        = 1;
  k_tex_desc.ArraySize        = 1;
  k_tex_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
  k_tex_desc.SampleDesc.Count = 1;
  k_tex_desc.Usage            = D3D11_USAGE_DEFAULT;
  k_tex_desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
  k_tex_desc.CPUAccessFlags   = 0;

  ID3D11Texture2D* k_com_tex{};

  D3D11_SUBRESOURCE_DATA k_sub_resource;
  k_sub_resource.pSysMem          = k_image.data;
  k_sub_resource.SysMemPitch      = k_tex_desc.Width * 4;
  k_sub_resource.SysMemSlicePitch = 0;
  auto k_r                        = k_g->CreateTexture2D(&k_tex_desc, &k_sub_resource, &k_com_tex);
  chick_true<doodle_error>(k_r == 0, DOODLE_LOC, "windows com 异常 {}", k_r);
  guard_win_ptr_delete l_a_delete{k_com_tex};

  D3D11_SHADER_RESOURCE_VIEW_DESC k_srv;
  ZeroMemory(&k_srv, sizeof(k_srv));
  k_srv.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
  k_srv.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
  k_srv.Texture2D.MipLevels       = k_tex_desc.MipLevels;
  k_srv.Texture2D.MostDetailedMip = 0;

  ID3D11ShaderResourceView* k_out_{nullptr};
  k_r = k_g->CreateShaderResourceView(k_com_tex, &k_srv, &(k_out_));
  chick_true<doodle_error>(k_r == 0, DOODLE_LOC, "windows com 异常 {}", k_r);
  //  winrt::com_ptr<ID3D11ShaderResourceView> k_l;
  in_handle.patch<image_icon>([&](image_icon& in) {
    in.image = std::shared_ptr<ID3D11ShaderResourceView>{k_out_, win_ptr_delete<ID3D11ShaderResourceView>{}};
  });

  return false;
}
bool image_loader::save(const entt::handle& in_handle) {
  auto k_reg = g_reg();
  chick_true<doodle_error>(k_reg->try_ctx<project>(), DOODLE_LOC, "缺失项目上下文");

  in_handle.get_or_emplace<image_icon>();

  return false;
}
std::shared_ptr<void> image_loader::screenshot() {
  // 获得全局GPU渲染对象
  auto k_d3d     = app::Get().d3dDevice;
  auto k_ctx     = app::Get().d3dDeviceContext;
  auto k_ded_swp = win::d3d_device::Get().g_pSwapChain;

  HRESULT k_r{};
  IDXGIOutputDuplication* l_duplication{};
#if 0
  {
    ID3D11Device* lDevice{};
    ID3D11DeviceContext* lImmediateContext{};
    IDXGIFactory* lDxgiFactory;
    k_r = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&lDxgiFactory);
    IDXGIAdapter* lDxgiAdapter;
    k_r = lDxgiFactory->EnumAdapters(0, &lDxgiAdapter);
    lDxgiFactory->Release();
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    k_r = D3D11CreateDevice(lDxgiAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                            0, featureLevelArray, 2,
                            D3D11_SDK_VERSION, &lDevice, nullptr, &lImmediateContext);
    IDXGIDevice* lDxgiDevice;
    k_r = lDevice->QueryInterface(IID_PPV_ARGS(&lDxgiDevice));
    //     IDXGIAdapter* lDxgiAdapter;
    //    (lDxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&lDxgiAdapter))
    lDxgiDevice->Release();
    IDXGIOutput* lDxgiOutput;
    k_r = lDxgiAdapter->EnumOutputs(0, &lDxgiOutput);
    lDxgiAdapter->Release();
    DXGI_OUTPUT_DESC OutputDesc{};
    k_r = lDxgiOutput->GetDesc(&OutputDesc);
    IDXGIOutput1* lDxgiOutput1;
    k_r = lDxgiOutput->QueryInterface(IID_PPV_ARGS(&lDxgiOutput1));
    lDxgiOutput->Release();
    k_r = lDxgiOutput1->DuplicateOutput(lDevice, &l_duplication);
    lDevice->Release();
    lDxgiOutput1->Release();
  }
#endif

  /// \brief 这个获取 IDXGIOutputDuplication 方法没有成功
#if 1
  {
    IDXGIDevice* l_dx_deve{};
    k_r = k_d3d->QueryInterface(IID_PPV_ARGS(&l_dx_deve));
    chick_true<doodle_error>(k_r == 0, DOODLE_LOC, "windows com 转换异常");
    guard_win_ptr_delete _guard_win_ptr_delete1{l_dx_deve};

    IDXGIAdapter* l_adapter{};
    k_r = l_dx_deve->GetParent(IID_PPV_ARGS(&l_adapter));
    chick_true<doodle_error>(k_r == 0, DOODLE_LOC, "windows com 转换异常");
    guard_win_ptr_delete _guard_win_ptr_delete2{l_adapter};

    IDXGIOutput* l_output{};
    k_r = l_adapter->EnumOutputs(1, &l_output);
    chick_true<doodle_error>(k_r == 0, DOODLE_LOC, "windows com 转换异常");
    guard_win_ptr_delete _guard_win_ptr_delete3{l_output};

    //    DXGI_OUTPUT_DESC l_desc{};
    //    k_r = l_output->GetDesc(&l_desc);

    IDXGIOutput1* l_output_1{};
    k_r = l_output->QueryInterface(IID_PPV_ARGS(&l_output_1));
    chick_true<doodle_error>(k_r == 0, DOODLE_LOC, "windows com 转换异常");
    guard_win_ptr_delete _guard_win_ptr_delete4{l_output_1};

    k_r = l_output_1->DuplicateOutput(k_d3d, &l_duplication);
    chick_true<doodle_error>(k_r == 0, DOODLE_LOC, "windows com 异常 {}", k_r);
  }
#endif
  guard_win_ptr_delete _guard_win_ptr_delete5{l_duplication};

#if 1
  DXGI_OUTDUPL_DESC l_dxgi_outdupl_desc{};
  l_duplication->GetDesc(&l_dxgi_outdupl_desc);
  D3D11_TEXTURE2D_DESC l_d_desc{};
  l_d_desc.Width              = l_dxgi_outdupl_desc.ModeDesc.Width;
  l_d_desc.Height             = l_dxgi_outdupl_desc.ModeDesc.Height;
  l_d_desc.Format             = l_dxgi_outdupl_desc.ModeDesc.Format;
  l_d_desc.ArraySize          = 1;
  l_d_desc.BindFlags          = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;
  l_d_desc.MiscFlags          = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
  l_d_desc.SampleDesc.Count   = 1;
  l_d_desc.SampleDesc.Quality = 0;
  l_d_desc.MipLevels          = 1;
  l_d_desc.CPUAccessFlags     = 0;
  l_d_desc.Usage              = D3D11_USAGE_DEFAULT;
#endif

  ID3D11Texture2D* l_tex{};
  k_r = k_d3d->CreateTexture2D(&l_d_desc, nullptr, &l_tex);
  guard_win_ptr_delete _guard_win_ptr_delete7{l_tex};

  DXGI_OUTDUPL_FRAME_INFO l_frame_info;
  IDXGIResource* DesktopResource{};
  k_r = l_duplication->AcquireNextFrame(INFINITE, &l_frame_info, &DesktopResource);
  chick_true<doodle_error>(k_r >= 0, DOODLE_LOC, "windows com 异常 {}", k_r);
  guard_win_ptr_delete _guard_win_ptr_delete6{DesktopResource};

  /// \brief 创建保存
  ID3D11Texture2D* l_image{};
  k_r = DesktopResource->QueryInterface(IID_PPV_ARGS(&l_image));
  chick_true<doodle_error>(k_r == 0, DOODLE_LOC, "windows com 异常 {}", k_r);
  guard_win_ptr_delete _guard_win_ptr_delete{l_image};

  D3D11_TEXTURE2D_DESC ThisDesc;
  l_image->GetDesc(&ThisDesc);



  k_ctx->CopyResource(l_tex, l_image);
  l_image->Release();

  /// \brief 保存图片
  k_r = DirectX::SaveWICTextureToFile(k_ctx, l_tex, GUID_ContainerFormatPng, L"D:/tmp/test_1_24.png");
  chick_true<doodle_error>(k_r == 0, DOODLE_LOC, "windows com 异常 {}", k_r);

  /// \brief 这里创建gpu资源返回给imgui
  D3D11_SHADER_RESOURCE_VIEW_DESC k_srv{};
  k_srv.Format                    = ThisDesc.Format;
  k_srv.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
  k_srv.Texture2D.MostDetailedMip = ThisDesc.MipLevels - 1;
  k_srv.Texture2D.MipLevels       = ThisDesc.MipLevels;

  ID3D11ShaderResourceView* k_out_{nullptr};
  k_r = k_d3d->CreateShaderResourceView(l_tex, &k_srv, &(k_out_));
  chick_true<doodle_error>(k_r == 0, DOODLE_LOC, "windows com 异常 {}", k_r);

  return std::shared_ptr<void>(k_out_, win_ptr_delete<ID3D11ShaderResourceView>{});
}
image_loader::~image_loader() = default;
}  // namespace doodle
