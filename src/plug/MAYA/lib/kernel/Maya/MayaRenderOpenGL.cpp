#include <lib/kernel/Maya/MayaRenderOpenGL.h>

#include <lib/kernel/Exception.h>

#include <sstream>

//maya ui
#include <Maya/M3dView.h>
//maya render
#include <Maya/MViewport2Renderer.h>
#include <Maya/MDrawContext.h>
///maya aim
#include <Maya/MAnimControl.h>

#define DOODLE_MAYA_CHICK(status) \
  if (status != MStatus::kSuccess) throw MayaError(status.errorString().asUTF8());

namespace doodle::motion::kernel {
void MayaRenderOpenGL::captureCallback(MHWRender::MDrawContext& context, void* clientData) {
  auto k_this = (MayaRenderOpenGL*)clientData;
  if (!k_this) {
    return;
  }

  // k_this->DebugPring(context);
  auto k_render = MHWRender::MRenderer::theRenderer();
  if (!k_render) {
    return;
  }

  //测试是否可以获得文件名称
  auto k_val = k_this->getFileName(k_this->p_current_time);
  if (!k_val) return;

  MString fileName{k_val.get()};

  const auto& k_color_target = context.getCurrentColorRenderTarget();
  auto k_ok                  = false;

  if (k_color_target) {
    auto k_texManager = k_render->getTextureManager();
    auto k_tex        = context.copyCurrentColorRenderTargetToTexture();
    if (k_tex) {
      auto status = k_texManager->saveTexture(k_tex, fileName);
      // auto k_rowPitch   = 0;
      // auto k_slicePitch = size_t{0};
      // auto k_data       = k_tex->rawData(k_rowPitch, k_slicePitch);
      // k_this->p_video->addFrame(k_data, 4);

      k_ok = (status == MStatus::kSuccess);

      k_texManager->releaseTexture(k_tex);
    }

    const auto k_targetManger = k_render->getRenderTargetManager();
    k_targetManger->releaseRenderTarget(k_color_target);
  }

  MString k_tmp{};
  if (!k_ok) {
    k_tmp.setUTF8("无法对目标进行颜色渲染: ");
    MGlobal::displayError(k_tmp + fileName);
  } else {
    k_tmp.setUTF8("捕获的颜色渲染目标到: ");
    MGlobal::displayInfo(k_tmp + fileName);
  }
}

MayaRenderOpenGL::MayaRenderOpenGL(uint32_t width, uint32_t height)
    : p_start_pos((double)0, MTime::uiUnit()),
      p_end_pos((double)30, MTime::uiUnit()),
      p_current_time(),
      p_width(std::move(width)),
      p_height(std::move(height)),
      p_post_render_notification_name("doodle_motion_openGl_name"),
      p_post_render_notification_semantic(
          MHWRender::MPassContext::kEndRenderSemantic),
      getFileName() {
}

MayaRenderOpenGL::~MayaRenderOpenGL() {
}

void MayaRenderOpenGL::save(const MTime& start, const MTime& end) {
  p_start_pos = start;
  p_end_pos   = end;

  auto k_status = MStatus{};

  auto k_render = MHWRender::MRenderer::theRenderer();
  if (!k_render) throw MayaNullptrError("MRenderer");

  auto k_view = M3dView::active3dView(&k_status);
  DOODLE_MAYA_CHICK(k_status);

  //创建视频输出
  // p_video = std::make_unique<MayaVideo>(p_file, p_width, p_height);

  k_render->addNotification(&captureCallback,
                            p_post_render_notification_name,
                            p_post_render_notification_semantic,
                            (void*)this);
  //设置尺寸覆盖
  k_render->setOutputTargetOverrideSize(p_width, p_height);
  //设置屏幕外显示
  k_render->setPresentOnScreen(false);
  for (p_current_time = p_start_pos;
       p_current_time <= p_end_pos;
       ++p_current_time) {
    MAnimControl::setCurrentTime(p_current_time);
    k_view.refresh(false, true);
  }
  k_render->removeNotification(p_post_render_notification_name,
                               p_post_render_notification_semantic);
  k_render->setPresentOnScreen(true);
  k_render->unsetOutputTargetOverrideSize();
}

}  // namespace doodle::motion::kernel

#undef DOODLE_MAYA_CHICK