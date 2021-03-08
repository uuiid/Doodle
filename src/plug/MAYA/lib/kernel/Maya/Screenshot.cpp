#include <lib/kernel/Maya/Screenshot.h>

#include <lib/kernel/Exception.h>
#include <lib/kernel/doodleFFmpeg.h>

// #include <boost/format.hpp>
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
Screenshot::Screenshot(FSys::path path)
    : p_file(path),
      p_start_pos((double)0, MTime::uiUnit()),
      p_end_pos((double)30, MTime::uiUnit()),
      p_current_time(),
      p_width(128),
      p_height(128),
      p_post_render_notification_name("doodle_motion_Screenshot_name"),
      p_post_render_notification_semantic(MHWRender::MPassContext::kEndRenderSemantic) {
}

void Screenshot::save(const MTime& start, const MTime& end) {
  auto k_av = avformat_alloc_context();

  auto k_status = MStatus{};

  auto k_render = MHWRender::MRenderer::theRenderer();
  if (!k_render) throw MayaNullptrError("MRenderer");

  auto k_view = M3dView::active3dView(&k_status);
  DOODLE_MAYA_CHICK(k_status);

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

void Screenshot::captureCallback(MHWRender::MDrawContext& context, void* clientData) {
  auto k_this = (Screenshot*)clientData;
  if (!k_this) {
    return;
  }

  // k_this->DebugPring(context);
  auto k_render = MHWRender::MRenderer::theRenderer();
  if (!k_render) {
    return;
  }

  std::cout << k_this->p_file.generic_u8string() << std::endl;

  MString fileName{};
  MString k_tmp{};
  k_tmp.setUTF8(k_this->p_file.parent_path().generic_u8string().c_str());
  fileName += k_tmp;
  fileName += "/";
  k_tmp.setUTF8(k_this->p_file.stem().generic_u8string().c_str());
  fileName += k_tmp;
  fileName += ".";

  std::stringstream str{};

  str << std::setw(5) << std::setfill('0') << (k_this->p_current_time.value());
  fileName += str.str().c_str();

  k_tmp.setUTF8(k_this->p_file.extension().generic_u8string().c_str());
  fileName += k_tmp;

  const auto& k_color_target = context.getCurrentColorRenderTarget();
  auto k_ok                  = false;

  if (k_color_target) {
    auto k_texManager = k_render->getTextureManager();
    auto k_tex        = context.copyCurrentColorRenderTargetToTexture();
    if (k_tex) {
      auto status = k_texManager->saveTexture(k_tex, fileName);
      k_ok        = (status == MStatus::kSuccess);

      k_texManager->releaseTexture(k_tex);
    }

    const auto k_targetManger = k_render->getRenderTargetManager();
    k_targetManger->releaseRenderTarget(k_color_target);
  }
  if (!k_ok) {
    k_tmp.setUTF8("无法对目标进行颜色渲染");
    MGlobal::displayError(k_tmp + fileName);
  } else {
    k_tmp.setUTF8("捕获的颜色渲染目标到");
    MGlobal::displayInfo(k_tmp + fileName);
  }
}

void Screenshot::DebugPring(MHWRender::MDrawContext& context) {
  const auto& k_ctx = context.getPassContext();
  const auto& k_id  = k_ctx.passIdentifier();
  const auto& k_sem = k_ctx.passSemantics();

  std::wcout << L"传递标识符 = " << k_id.asWChar() << std::endl;
  for (uint32_t i = 0; i < k_sem.length(); ++i) {
    std::wcout << L"传递语义 : " << k_sem[i].asWChar() << std::endl;
  }
}

Screenshot::~Screenshot() {
}
}  // namespace doodle::motion::kernel