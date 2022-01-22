//
// Created by TD on 2022/1/21.
//

#include "image_loader.h"
#include <opencv2/opencv.hpp>
#include <metadata/project.h>
#include <metadata/image_icon.h>
#include <app/app.h>

#include <winrt/base.h>

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
  guard_win_ptr_delete<ID3D11Texture2D> l_a_delete{k_com_tex};

  D3D11_SUBRESOURCE_DATA k_sub_resource;
  k_sub_resource.pSysMem          = k_image.data;
  k_sub_resource.SysMemPitch      = k_tex_desc.Width * 4;
  k_sub_resource.SysMemSlicePitch = 0;
  auto k_r                        = k_g->CreateTexture2D(&k_tex_desc, &k_sub_resource, &k_com_tex);
  chick_true<doodle_error>(k_r == 0, DOODLE_LOC, "windows com 异常 {}", k_r);

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
image_loader::~image_loader() = default;
}  // namespace doodle
