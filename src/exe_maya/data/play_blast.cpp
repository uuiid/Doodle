//
// Created by TD on 2021/11/22.
//

#include "play_blast.h"

#include "doodle_core/exception/exception.h"
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/metadata/move_create.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/thread_pool/image_to_movie.h>

#include <doodle_lib/long_task/image_to_move.h>

#include "boost/numeric/conversion/cast.hpp"

#include "maya_plug/main/maya_plug_fwd.h"
#include <maya_plug/data/maya_camera.h>

#include "maya/MApiNamespace.h"
#include "maya/MRenderTargetManager.h"
#include "maya/MStatus.h"
#include "maya/MString.h"
#include "maya/MTextureManager.h"
#include <cstdint>
#include <fmt/chrono.h>
#include <fmt/ostream.h>
#include <maya/M3dView.h>
#include <maya/MAnimControl.h>
#include <maya/MDrawContext.h>
#include <maya/MFileIO.h>
#include <maya/MGlobal.h>
#include <wil/registry.h>
namespace doodle::maya_plug {

MString play_blast::p_post_render_notification_name{"doodle_maya_notification"};
MString play_blast::k_play_blast_tex{"doodle_maya_play_blast_tex"};

void play_blast::captureCallback(MHWRender::MDrawContext& context, void* clientData) {
  using tex_ptr_t = std::shared_ptr<MTexture>;

  MStatus k_s{};
  auto* self     = static_cast<play_blast*>(clientData);
  auto* k_render = MHWRender::MRenderer::theRenderer();
  if (!self || !k_render) return;

  auto k_p = self->get_file_path(self->p_current_time);
  if (k_p.empty()) return;

  MString const k_path{d_str{k_p.generic_string()}};

  auto* l_tex_manager = k_render->getTextureManager();
  tex_ptr_t const l_tex_ptr{context.copyCurrentColorRenderTargetToTexture(), [=](MTexture* in_texture) {
                              if (in_texture) l_tex_manager->releaseTexture(in_texture);
                            }};
  if (l_tex_ptr) {
    k_s = l_tex_manager->saveTexture(l_tex_ptr.get(), k_path);
    if (!k_s) {
      MGlobal::displayError(k_s.errorString());
    }
  }

  DOODLE_LOG_INFO("save texture: {}", k_path);
}

play_blast::play_blast()
    : p_save_path(core_set::get_set().get_cache_root("maya_play_blast")),
      p_eps(),
      p_shot(),
      p_current_time(),
      p_uuid() {}
FSys::path play_blast::get_file_dir() const {
  auto k_cache_path = core_set::get_set().get_cache_root("maya_play_blast/tmp");
  k_cache_path /= p_uuid.substr(0, 3);
  if (!FSys::exists(k_cache_path)) {
    FSys::create_directories(k_cache_path);
  }
  return k_cache_path;
}

FSys::path play_blast::get_file_path(const MTime& in_time) const {
  auto k_cache_path = get_file_dir();
  k_cache_path /= fmt::format("{}_{:05d}.png", p_uuid, boost::numeric_cast<std::int32_t>(in_time.as(MTime::uiUnit())));
  return k_cache_path;
}
FSys::path play_blast::get_file_path() const {
  auto k_cache_path = get_file_dir();
  k_cache_path /= fmt::format("{}_", p_uuid);
  return k_cache_path;
}
FSys::path play_blast::set_save_path(const FSys::path& in_save_path) {
  p_save_path = in_save_path;
  return get_out_path();
}
FSys::path play_blast::set_save_dir(const FSys::path& in_save_dir) {
  p_save_path = in_save_dir / p_save_path.filename();
  return get_out_path();
}

FSys::path play_blast::set_save_filename(const FSys::path& in_save_filename) {
  p_save_path.remove_filename() /= in_save_filename;
  return get_out_path();
}

FSys::path play_blast::get_out_path() const {
  if (!FSys::exists(p_save_path.parent_path())) FSys::create_directories(p_save_path.parent_path());
  // k_cache_path /= fmt::format("{}_{}.mp4", p_eps, p_shot);
  return p_save_path;
}



MStatus play_blast::play_blast_(const MTime& in_start, const MTime& in_end) {
  p_uuid = core_set::get_set().get_uuid_str();
  MStatus k_s{};

  auto k_cam = maya_camera::conjecture();
  k_cam.set_render_cam();
  k_cam.set_play_attr();
  // 设置一些固定属性
  movie::image_attr l_image{};
  l_image.gamma_t = 0.9;
  /// \brief 相机名称
  l_image.watermarks_attr.emplace_back(
      k_cam.get_transform_name(), 0.1, 0.1, movie::image_watermark::rgba_t{25, 220, 2}
  );

  /// \brief 绘制摄像机avo
  l_image.watermarks_attr.emplace_back(
      fmt::format("FOV: {:.3f}", k_cam.focalLength()), 0.91, 0.1, movie::image_watermark::rgba_t{25, 220, 2}
  );
  /// \brief 当前时间节点
  l_image.watermarks_attr.emplace_back(
      fmt::format("{:%Y-%m-%d %H:%M:%S}", chrono::floor<chrono::minutes>(chrono::system_clock::now())), 0.1, 0.91,
      movie::image_watermark::rgba_t{25, 220, 2}
  );

  //  k_s = MGlobal::executeCommand(R"(colorManagementPrefs -e -outputTransformEnabled true -outputTarget "renderer";
  //  colorManagementPrefs -e -outputUseViewTransform -outputTarget "renderer";)");
  //  CHECK_MSTATUS_AND_RETURN_IT(k_s);
  //
  //  CHECK_MSTATUS_AND_RETURN_IT(k_s);
  //  MGlobal::executeCommand(R"(colorManagementPrefs -e -outputTransformEnabled false -outputTarget "renderer";)");
  std::vector<movie::image_attr> l_handle_list{};

  {
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) return MStatus::kFailure;

    //    renderer->addNotification(
    //        &play_blast::captureCallback, p_post_render_notification_name,
    //        MHWRender::MPassContext::kEndRenderSemantic, reinterpret_cast<void*>(this)
    //    );

    renderer->setOutputTargetOverrideSize(1920, 1080);
    renderer->setPresentOnScreen(false);
    const auto l_cam_path  = k_cam.get_full_name();

    using target_ptr_t     = std::shared_ptr<MHWRender::MRenderTarget>;
    using tex_ptr_t        = std::shared_ptr<MTexture>;
    using tex_data_ptr_t   = std::shared_ptr<void>;

    auto* l_tex_manager    = renderer->getTextureManager();
    auto* l_target_manager = renderer->getRenderTargetManager();
    MHWRender::MRenderTarget* l_rt{l_target_manager->acquireRenderTarget(
        MRenderTargetDescription{k_play_blast_tex, 1920, 1080, 1, ::MHWRender::MRasterFormat ::kB8G8R8A8, 1, false}
    )};
    target_ptr_t const l_t{l_rt, [=](MHWRender::MRenderTarget* in_target) {
                             if (in_target) l_target_manager->releaseRenderTarget(in_target);
                           }};
    //    MHWRender::MRenderTarget* l_rt{l_target_manager->acquireRenderTargetFromScreen(k_play_blast_tex)};

    DOODLE_LOG_INFO("set output camera: {}", l_cam_path);
    // 开始排屏, 并且开始将结果属性写入
    /// \brief 当前帧和总帧数
    auto k_len = in_end - in_start + 1;
    for (MTime i{in_start}; i <= in_end; ++i) {
      MAnimControl::setCurrentTime(i);

      renderer->render(d_str{fmt::format("batch:{}", l_cam_path)}, &l_rt, 1);

      if (l_rt) {
        int l_row_pitch      = 0;
        size_t l_slice_pitch = 0;
        MHWRender::MRenderTargetDescription l_target_desc;
        tex_data_ptr_t const l_target_data{l_rt->rawData(l_row_pitch, l_slice_pitch), [](void* in_data) {
                                             MHWRender::MRenderTarget::freeRawData(in_data);
                                           }};

        l_rt->targetDescription(l_target_desc);
        MHWRender::MTextureDescription l_texture_desc;
        l_texture_desc.fWidth         = l_target_desc.width();
        l_texture_desc.fHeight        = l_target_desc.height();
        l_texture_desc.fDepth         = 1;
        l_texture_desc.fBytesPerRow   = l_row_pitch;
        l_texture_desc.fBytesPerSlice = l_slice_pitch;
        l_texture_desc.fMipmaps       = 1;
        l_texture_desc.fArraySlices   = l_target_desc.arraySliceCount();
        l_texture_desc.fFormat        = l_target_desc.rasterFormat();
        l_texture_desc.fTextureType   = MHWRender::kImage2D;
        l_texture_desc.fEnvMapType    = MHWRender::MEnvironmentMapType::kEnvNone;
        tex_ptr_t const l_tex{
            l_tex_manager->acquireTexture({}, l_texture_desc, l_target_data.get()),
            [=](MTexture* in_texture) {
              if (in_texture) l_tex_manager->releaseTexture(in_texture);
            }
        };
        auto k_p = get_file_path(i);

        MString const k_path{d_str{k_p.generic_string()}};
        l_tex_manager->saveTexture(l_tex.get(), k_path);

        // 开始设置属性
        movie::image_attr l_image_tmp = l_image;
        l_image_tmp.path_attr         = k_p;
        l_image_tmp.gamma_t           = 0.9;
        l_image_tmp.num_attr          = i.value();
        l_image_tmp.watermarks_attr.emplace_back(
            fmt::format("{}/{}", i.value(), boost::numeric_cast<std::int32_t>(k_len.value())), 0.5, 0.1,
            movie::image_watermark::rgba_t{25, 220, 2}
        );
        l_handle_list.push_back(std::move(l_image_tmp));
      }
    }
    //    renderer->removeNotification(p_post_render_notification_name, MHWRender::MPassContext::kEndRenderSemantic);
    // 恢复关闭屏幕上的更新
    renderer->setPresentOnScreen(true);
    // 禁用目标尺寸覆盖
    renderer->unsetOutputTargetOverrideSize();
  }

  auto l_out_path = detail::create_out_path(get_out_path(), p_eps, p_shot, nullptr);
  detail::create_move(l_out_path, spdlog::default_logger(), l_handle_list);
  auto k_f = get_file_dir();
  default_logger_raw()->log(log_loc(), spdlog::level::info, "完成视频合成 {} , 并删除图片 {}", l_out_path, k_f);
  try {
    FSys::remove_all(k_f);
  } catch (const std::exception& e) {
    DOODLE_LOG_ERROR("删除图片失败 {}", e.what());
  }

  return k_s;
}

bool play_blast::conjecture_ep_sc() {
  FSys::path p_current_path{MFileIO::currentFile().asUTF8()};
  auto k_r = p_eps.analysis(p_current_path) && p_shot.analysis(p_current_path);
  set_save_filename(p_current_path.filename().replace_extension(".mp4"));
  return k_r;
}

}  // namespace doodle::maya_plug
