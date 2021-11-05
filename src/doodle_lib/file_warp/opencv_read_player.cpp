//
// Created by TD on 2021/11/04.
//

#include "opencv_read_player.h"

#include <d3d11.h>
#include <doodle_lib/doodle_app.h>
#include <doodle_lib/exception/exception.h>

#include <opencv2/opencv.hpp>

namespace doodle {

namespace {
template <class T>
struct win_ptr_delete {
  void operator()(T* ptr) const {
    ptr->Release();
  }
};

struct frame {
  std::unique_ptr<ID3D11ShaderResourceView,
                  win_ptr_delete<ID3D11ShaderResourceView>>
      p_d3d_view;
  std::uint32_t p_frame;
  std::uint32_t p_width;
  std::uint32_t p_height;
  frame() = default;
  DOODLE_MOVE(frame);
};

}  // namespace

class opencv_read_player::impl {
 public:
  impl(){};
  ~impl(){};
  cv::VideoCapture p_video;
  std::map<std::uint32_t, frame> p_image;
};

opencv_read_player::opencv_read_player()
    : p_data(new impl{}) {
  // p_data = std::make_unique<>();
}

opencv_read_player::~opencv_read_player() = default;
DOODLE_MOVE_CPP(opencv_read_player)

bool opencv_read_player::load_frame(std::int32_t in_frame) {
  if (!p_data->p_video.isOpened())
    throw doodle_error{"没有打开的视频"};

  if (p_data->p_image.count(in_frame) == 1) {
    return true;
  }

  frame k_f{};
  k_f.p_height = boost::numeric_cast<std::int32_t>(p_data->p_video.get(cv::CAP_PROP_FRAME_HEIGHT));
  k_f.p_width  = boost::numeric_cast<std::int32_t>(p_data->p_video.get(cv::CAP_PROP_FRAME_WIDTH));
  k_f.p_frame  = in_frame;

  cv::Mat p_mat{};
  p_data->p_video.set(cv::CAP_PROP_POS_FRAMES, boost::numeric_cast<std::double_t>(in_frame));
  if (!p_data->p_video.read(p_mat))
    throw doodle_error{"无法读取帧"};
  /// 转换图像
  cv::cvtColor(p_mat, p_mat, cv::COLOR_BGR2RGBA);
  //
  ID3D11ShaderResourceView* k_out_{nullptr};

  auto k_g = doodle_app::Get()->p_pd3dDevice;
  D3D11_TEXTURE2D_DESC k_tex_desc{};
  k_tex_desc.Width            = k_f.p_height;
  k_tex_desc.Height           = k_f.p_width;
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

  k_g->CreateShaderResourceView(k_tex, &k_srv, &(k_out_));
  k_tex->Release();
  k_f.p_d3d_view.reset(k_out_);
  p_data->p_image.insert({in_frame, std::move(k_f)});

  return true;
}

bool opencv_read_player::clear_cache() {
  return true;
}

bool opencv_read_player::is_open() const {
  return p_data->p_video.isOpened();
}

bool opencv_read_player::open_file(const FSys::path& in_path) {
  return p_data->p_video.open(in_path.generic_string());
}

std::tuple<void*, std::pair<std::int32_t, std::int32_t>> opencv_read_player::read(std::int32_t in_frame) {
  load_frame(in_frame);
  auto& k_ = p_data->p_image[in_frame];
  return std::make_tuple((void*)(k_.p_d3d_view.get()), std::make_pair(k_.p_width, k_.p_height));
}

}  // namespace doodle