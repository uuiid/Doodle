//
// Created by TD on 2022/1/21.
//

#include "image_loader.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>

#include <metadata/project.h>
#include <metadata/image_icon.h>
#include <core/core_set.h>
#include <app/app.h>
#include <platform/win/wnd_proc.h>

//#include <DirectXTK/ScreenGrab.h>
//#include <wincodec.h>

//#include <winrt/base.h>
//#include <atlbase.h>
//#include <atlwin.h>
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

}  // namespace

class image_loader::impl {
 public:
};

image_loader::image_loader()
    : p_i() {
  if (auto k_ctx = g_reg()->try_ctx<cache>(); !k_ctx)
    g_reg()->set<cache>();
}

bool image_loader::load(const entt::handle& in_handle) {
  chick_true<doodle_error>(in_handle.any_of<image_icon>(), DOODLE_LOC, "缺失图标组件");
  auto k_reg = g_reg();
  chick_true<doodle_error>(k_reg->try_ctx<project>(), DOODLE_LOC, "缺失项目上下文");

  auto l_local_path = k_reg->ctx<project>().p_path / "image" / in_handle.get<image_icon>().path;

  if (exists(l_local_path) &&
      is_regular_file(l_local_path) &&
      l_local_path.extension() == ".png") {
    auto k_image = cv::imread(l_local_path.generic_string());
    chick_true<doodle_error>(!k_image.empty(), DOODLE_LOC, "open cv not read image");
    cv::cvtColor(k_image, k_image, cv::COLOR_BGR2RGBA);
    auto k_sh = cv_to_d3d(k_image, false);
    in_handle.patch<image_icon>([&](image_icon& in) {
      in.image = k_sh;
    });
  } else {
    in_handle.patch<image_icon>([&](image_icon& in) {
      in.image = error_image();
    });
  }

  return false;
}
bool image_loader::save(const entt::handle& in_handle,
                        const cv::Mat& in_image,
                        const cv::Rect2f& in_rect) {
  auto k_reg = g_reg();
  chick_true<doodle_error>(k_reg->try_ctx<project>(), DOODLE_LOC, "缺失项目上下文");

  auto& k_icon = in_handle.get_or_emplace<image_icon>();

  auto k_image = in_image(in_rect).clone();

  k_icon.path  = core_set::getSet().get_uuid_str() + ".png";
  auto k_path  = k_reg->ctx<project>().make_path("image") / k_icon.path;

  cv::imwrite(k_path.generic_string(), k_image);
  k_icon.image   = cv_to_d3d(k_image);
  k_icon.size2d_ = in_rect.size();
  return true;
}

std::shared_ptr<void> image_loader::cv_to_d3d(const cv::Mat& in_mat) const {
  return cv_to_d3d(in_mat, true);
}

cv::Mat image_loader::screenshot() {
#if 0
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


  k_ctx->CopyResource(l_tex, l_image);
  l_duplication->ReleaseFrame();
  D3D11_TEXTURE2D_DESC ThisDesc;
  l_tex->GetDesc(&ThisDesc);

  /// \brief 保存图片
//  k_r = DirectX::SaveWICTextureToFile(k_ctx, l_tex, GUID_ContainerFormatPng, L"D:/tmp/test_1_24.png");
//  chick_true<doodle_error>(k_r == 0, DOODLE_LOC, "windows com 异常 {}", k_r);

  /// \brief 这里创建gpu资源返回给imgui
  D3D11_SHADER_RESOURCE_VIEW_DESC k_srv{};
  k_srv.Format                    = ThisDesc.Format;
  k_srv.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
  k_srv.Texture2D.MostDetailedMip = ThisDesc.MipLevels - 1;
  k_srv.Texture2D.MipLevels       = ThisDesc.MipLevels;

  ID3D11ShaderResourceView* k_out_{nullptr};

  k_r = k_d3d->CreateShaderResourceView(l_image, &k_srv, &(k_out_));
  chick_true<doodle_error>(k_r == 0, DOODLE_LOC, "windows com 异常 {}", k_r);

  return std::shared_ptr<void>(k_out_, win_ptr_delete<ID3D11ShaderResourceView>{});
#endif
  return win::get_screenshot();
}

std::shared_ptr<void> image_loader::default_image() const {
  if (auto k_c = g_reg()->ctx<cache>().default_image; !k_c) {
    int fontFace     = cv::HersheyFonts::FONT_HERSHEY_COMPLEX;
    double fontScale = 1;
    int thickness    = 2;
    int baseline     = 0;
    {
      /// @brief 加载默认图片
      auto textSize = cv::getTextSize({"no"}, fontFace, fontScale, thickness, &baseline);
      cv::Mat k_mat{64, 64, CV_8UC4, cv::Scalar{0, 0, 0, 255}};

      cv::Point textOrg((k_mat.cols - textSize.width) * 0.5,
                        (k_mat.rows + textSize.height) * 0.5);

      cv::putText(k_mat, "no", textOrg, fontFace, fontScale,
                  {255, 255, 255, 255}, thickness, cv::LineTypes::LINE_AA);
      auto k_def                          = cv_to_d3d(k_mat);
      g_reg()->ctx<cache>().default_image = k_def;
      return k_def;
    }
  }
  return g_reg()->ctx<cache>().default_image;
}
std::shared_ptr<void> image_loader::error_image() const {
  if (auto k_c = g_reg()->ctx<cache>().error_image; !k_c) {
    int fontFace     = cv::HersheyFonts::FONT_HERSHEY_COMPLEX;
    double fontScale = 1;
    int thickness    = 2;
    int baseline     = 0;
    {
      /// @brief 加载错误图片
      auto textSize = cv::getTextSize({"err"}, fontFace, fontScale, thickness, &baseline);
      cv::Mat k_mat{64, 64, CV_8UC4, cv::Scalar{0, 0, 0, 255}};

      cv::Point textOrg((k_mat.cols - textSize.width) * 0.5,
                        (k_mat.rows + textSize.height) * 0.5);

      cv::putText(k_mat, "err", textOrg, fontFace, fontScale,
                  {20, 0, 255, 255}, thickness, cv::LineTypes::LINE_AA);
      auto k_def                        = cv_to_d3d(k_mat);
      g_reg()->ctx<cache>().error_image = k_def;
    }
  }
  return g_reg()->ctx<cache>().error_image;
}
std::shared_ptr<void> image_loader::cv_to_d3d(const cv::Mat& in_mat, bool convert_toRGBA) const {
  // 获得全局GPU渲染对象
  auto k_g = app::Get().d3dDevice;
  if (convert_toRGBA)
    /// \brief 转换图像
    cv::cvtColor(in_mat, in_mat, cv::COLOR_BGR2RGBA);

  D3D11_TEXTURE2D_DESC k_tex_desc{};
  k_tex_desc.Width            = in_mat.cols;
  k_tex_desc.Height           = in_mat.rows;
  k_tex_desc.MipLevels        = 1;
  k_tex_desc.ArraySize        = 1;
  k_tex_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
  k_tex_desc.SampleDesc.Count = 1;
  k_tex_desc.Usage            = D3D11_USAGE_DEFAULT;
  k_tex_desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
  k_tex_desc.CPUAccessFlags   = 0;

  ID3D11Texture2D* k_com_tex{};

  D3D11_SUBRESOURCE_DATA k_sub_resource;
  k_sub_resource.pSysMem          = in_mat.data;
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
  if (convert_toRGBA)
    /// \brief 转换图像
    cv::cvtColor(in_mat, in_mat, cv::COLOR_RGB2BGRA);
  return std::shared_ptr<void>{k_out_, win_ptr_delete<ID3D11ShaderResourceView>{}};
}
bool image_loader::save(const entt::handle& in_handle, const FSys::path& in_path) {
  auto k_reg = g_reg();
  chick_true<doodle_error>(k_reg->try_ctx<project>(), DOODLE_LOC, "缺失项目上下文");
  chick_true<doodle_error>(exists(in_path), DOODLE_LOC, "文件不存在");

  auto& k_icon = in_handle.get_or_emplace<image_icon>();
  k_icon.path  = core_set::getSet().get_uuid_str() + ".png";
  auto k_path  = k_reg->ctx<project>().make_path("image") / k_icon.path;

  FSys::copy(in_path, k_path, FSys::copy_options::overwrite_existing);
  auto l_mat   = cv::imread(k_path.generic_string());
  k_icon.image = cv_to_d3d(l_mat);
  k_icon.size2d_ = l_mat.size();
  return true;
}
image_loader::~image_loader() = default;
}  // namespace doodle
