#include <lib/kernel/Maya/Screenshot.h>

#include <lib/kernel/Exception.h>
#include <lib/kernel/doodleFFmpeg.h>
#include <lib/kernel/Maya/MayaRenderOpenGL.h>

#include <sstream>
#include <Maya/MGlobal.h>
#include <Maya/MAnimControl.h>

#define DOODLE_MAYA_CHICK(status) \
  if (status != MStatus::kSuccess) throw MayaError(status.errorString().asUTF8());

namespace doodle::motion::kernel {
Screenshot::Screenshot(FSys::path path)
    : p_file(path),
      p_width(128),
      p_height(128),
      p_view(std::make_unique<MayaRenderOpenGL>(p_width, p_height)) {
}

void Screenshot::save() {
  p_view->getFileName.connect([=](const MTime& time) -> MString {
    MString fileName{};
    MString k_tmp{};
    k_tmp.setUTF8(this->p_file.parent_path().generic_u8string().c_str());
    fileName += k_tmp;
    fileName += "/";
    k_tmp.setUTF8(this->p_file.stem().generic_u8string().c_str());
    fileName += k_tmp;
    // fileName += ".";
    // std::stringstream str{};
    // str << std::setw(5) << std::setfill('0') << (time.value());
    // fileName += str.str().c_str();

    k_tmp.setUTF8(this->p_file.extension().generic_u8string().c_str());
    fileName += k_tmp;
    return fileName;
  });
  p_view->save(MAnimControl::currentTime(), MAnimControl::currentTime());
}

// void Screenshot::captureCallback(MHWRender::MDrawContext& context, void* clientData) {
//   auto k_this = (Screenshot*)clientData;
//   if (!k_this) {
//     return;
//   }

//   // k_this->DebugPring(context);
//   auto k_render = MHWRender::MRenderer::theRenderer();
//   if (!k_render) {
//     return;
//   }

//   std::cout << k_this->p_file.generic_u8string() << std::endl;

//   MString fileName{};
//   MString k_tmp{};
//   k_tmp.setUTF8(k_this->p_file.parent_path().generic_u8string().c_str());
//   fileName += k_tmp;
//   fileName += "/";
//   k_tmp.setUTF8(k_this->p_file.stem().generic_u8string().c_str());
//   fileName += k_tmp;
//   fileName += ".";

//   std::stringstream str{};

//   str << std::setw(5) << std::setfill('0') << (k_this->p_current_time.value());
//   fileName += str.str().c_str();

//   k_tmp.setUTF8(k_this->p_file.extension().generic_u8string().c_str());
//   fileName += k_tmp;

//   const auto& k_color_target = context.getCurrentColorRenderTarget();
//   auto k_ok                  = false;

//   if (k_color_target) {
//     auto k_texManager = k_render->getTextureManager();
//     auto k_tex        = context.copyCurrentColorRenderTargetToTexture();
//     if (k_tex) {
//       auto status = k_texManager->saveTexture(k_tex, fileName);
//       // auto k_rowPitch   = 0;
//       // auto k_slicePitch = size_t{0};
//       // auto k_data       = k_tex->rawData(k_rowPitch, k_slicePitch);
//       // k_this->p_video->addFrame(k_data, 4);

//       k_ok = (status == MStatus::kSuccess);

//       k_texManager->releaseTexture(k_tex);
//     }

//     const auto k_targetManger = k_render->getRenderTargetManager();
//     k_targetManger->releaseRenderTarget(k_color_target);
//   }
//   if (!k_ok) {
//     k_tmp.setUTF8("无法对目标进行颜色渲染");
//     MGlobal::displayError(k_tmp + fileName);
//   } else {
//     k_tmp.setUTF8("捕获的颜色渲染目标到");
//     MGlobal::displayInfo(k_tmp + fileName);
//   }
// }

}  // namespace doodle::motion::kernel

#undef DOODLE_MAYA_CHICK