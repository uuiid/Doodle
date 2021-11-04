//
// Created by TD on 2021/11/04.
//

#include "opencv_read_player.h"

#include <d3d11.h>
#include <doodle_lib/doodle_app.h>
#include <doodle_lib/exception/exception.h>

#include <opencv2/opencv.hpp>
namespace doodle {
opencv_read_player::opencv_read_player()
    : p_video() {
}

bool opencv_read_player::is_open() const {
  return p_video.isOpened();
}

bool opencv_read_player::open_file(const FSys::path& in_path) {
  return p_video.open(in_path.generic_string());
}

std::tuple<void*, std::pair<std::int32_t, std::int32_t> > opencv_read_player::read(std::int32_t in_frame) {
  if (!p_video.isOpened())
    throw doodle_error{"没有打开的视频"};

  std::int32_t k_width{boost::numeric_cast<std::int32_t>(p_video.get(cv::CAP_PROP_FRAME_WIDTH))};
  std::int32_t k_height{boost::numeric_cast<std::int32_t>(p_video.get(cv::CAP_PROP_FRAME_HEIGHT))};
  cv::Mat p_mat{};
  p_video.set(cv::CAP_PROP_POS_FRAMES, boost::numeric_cast<std::double_t>(in_frame));
  if (p_video.read(p_mat)) {
    /// 转换图像
    cv::cvtColor(p_mat, p_mat, cv::COLOR_BGR2RGBA);
    //
    ID3D11ShaderResourceView* _out_{nullptr};
    ID3D11ShaderResourceView** out_srv{&_out_};

    auto k_g = doodle_app::Get()->p_pd3dDevice;
    D3D11_TEXTURE2D_DESC k_tex_desc{};
    k_tex_desc.Width            = k_width;
    k_tex_desc.Height           = k_height;
    k_tex_desc.MipLevels        = 1;
    k_tex_desc.ArraySize        = 1;
    k_tex_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    k_tex_desc.SampleDesc.Count = 1;
    k_tex_desc.Usage            = D3D11_USAGE_DEFAULT;
    k_tex_desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
    k_tex_desc.CPUAccessFlags   = 0;

    ID3D11Texture2D* k_tex      = nullptr;

    D3D11_SUBRESOURCE_DATA k_sub_resource;
    k_sub_resource.pSysMem          = p_mat.data;
    k_sub_resource.SysMemPitch      = k_tex_desc.Width * 4;
    k_sub_resource.SysMemSlicePitch = 0;
    k_g->CreateTexture2D(&k_tex_desc, &k_sub_resource, &k_tex);

    D3D11_SHADER_RESOURCE_VIEW_DESC k_srv;
    ZeroMemory(&k_srv, sizeof(k_srv));
    k_srv.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
    k_srv.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    k_srv.Texture2D.MipLevels       = k_tex_desc.MipLevels;
    k_srv.Texture2D.MostDetailedMip = 0;

    k_g->CreateShaderResourceView(k_tex, &k_srv, out_srv);
    k_tex->Release();
    return std::make_tuple(
        (void*)_out_,
        std::pair<std::int32_t, std::int32_t>{
            k_width,
            k_height});
  }
  return {};
}

}  // namespace doodle