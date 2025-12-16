//
// Created by TD on 2021/11/22.
//

#include "play_blast.h"

#include "doodle_core/exception/exception.h"
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/metadata/move_create.h>
#include <doodle_core/metadata/user.h>

#include <doodle_lib/long_task/image_to_move.h>

#include "boost/numeric/conversion/cast.hpp"

#include "maya_plug/data/maya_conv_str.h"
#include "maya_plug/exception/exception.h"
#include "maya_plug/main/maya_plug_fwd.h"
#include <maya_plug/data/maya_camera.h>

#include "maya/MRenderTargetManager.h"
#include "maya/MStatus.h"
#include "maya/MString.h"
#include "maya/MTextureManager.h"
#include <cstdint>
#include <filesystem>
#include <fmt/chrono.h>
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

play_blast::play_blast() : p_current_time(), p_uuid() {}
FSys::path play_blast::get_file_dir() const {
  auto k_cache_path = core_set::get_set().get_cache_root("maya_play_blast");
  k_cache_path /= p_uuid;
  if (!FSys::exists(k_cache_path)) {
    FSys::create_directories(k_cache_path);
  }
  return k_cache_path;
}

FSys::path play_blast::get_file_path(const MTime& in_time) const {
  auto k_cache_path = get_file_dir();
  k_cache_path /= fmt::format("image_{:05d}.png", boost::numeric_cast<std::int32_t>(in_time.as(MTime::uiUnit())));
  return k_cache_path;
}

FSys::path play_blast::play_blast_(const MTime& in_start, const MTime& in_end, const image_size& in_size) {
  p_uuid = core_set::get_set().get_uuid_str();
  MGlobal::executeCommand("hwRenderLoad;");
  MGlobal::executeCommand(R"(setAttr "hardwareRenderingGlobals.floatingPointRTEnable" 0;)");
  MGlobal::executeCommand(R"(setAttr "hardwareRenderingGlobals.multiSampleEnable" 0;)");
  MGlobal::executeCommand(R"(setAttr "hardwareRenderingGlobals.ssaoEnable" 0;)");
  MGlobal::executeCommand(R"(setAttr "hardwareRenderingGlobals.textureMaxResMode" 0;)");
  MStatus k_s{};

  auto k_cam = maya_camera::conjecture();
  k_cam.set_render_cam();
  k_cam.set_play_attr();
  //  k_s = MGlobal::executeCommand(R"(colorManagementPrefs -e -outputTransformEnabled true -outputTarget "renderer";
  //  colorManagementPrefs -e -outputUseViewTransform -outputTarget "renderer";)");
  //  CHECK_MSTATUS_AND_RETURN_IT(k_s);
  //
  //  CHECK_MSTATUS_AND_RETURN_IT(k_s);
  //  MGlobal::executeCommand(R"(colorManagementPrefs -e -outputTransformEnabled false -outputTarget "renderer";)");
  std::vector<movie::image_attr> l_handle_list{};

  {
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    DOODLE_CHICK(renderer != nullptr, "无法获取渲染器");

    //    renderer->addNotification(
    //        &play_blast::captureCallback, p_post_render_notification_name,
    //        MHWRender::MPassContext::kEndRenderSemantic, reinterpret_cast<void*>(this)
    //    );

    renderer->setOutputTargetOverrideSize(in_size.width, in_size.height);
    renderer->setPresentOnScreen(false);
    const auto l_cam_path  = k_cam.get_full_name();

    using target_ptr_t     = std::shared_ptr<MHWRender::MRenderTarget>;
    using tex_ptr_t        = std::shared_ptr<MTexture>;
    using tex_data_ptr_t   = std::shared_ptr<void>;

    auto* l_tex_manager    = renderer->getTextureManager();
    auto* l_target_manager = renderer->getRenderTargetManager();
    MHWRender::MRenderTarget* l_rt{l_target_manager->acquireRenderTarget(
        MRenderTargetDescription{
            k_play_blast_tex, boost::numeric_cast<std::uint32_t>(in_size.width),
            boost::numeric_cast<std::uint32_t>(in_size.height), 1, ::MHWRender::MRasterFormat ::kB8G8R8A8, 1, false
        }
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

      DOODLE_CHICK(renderer->render(conv::to_ms(fmt::format("batch:{}", l_cam_path)), &l_rt, 1), "渲染失败");

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
            l_tex_manager->acquireTexture({}, l_texture_desc, l_target_data.get()), [=](MTexture* in_texture) {
              if (in_texture) l_tex_manager->releaseTexture(in_texture);
            }
        };
        auto k_p = get_file_path(i);
        MString const k_path = conv::to_ms(k_p.generic_string());
        maya_chick(l_tex_manager->saveTexture(l_tex.get(), k_path));
      }
    }
    //    renderer->removeNotification(p_post_render_notification_name, MHWRender::MPassContext::kEndRenderSemantic);
    // 恢复关闭屏幕上的更新
    renderer->setPresentOnScreen(true);
    // 禁用目标尺寸覆盖
    renderer->unsetOutputTargetOverrideSize();
  }

  return get_file_dir();
}

}  // namespace doodle::maya_plug
