#pragma once

#include <lib/MotionGlobal.h>

//maya api
#include <Maya/MGlobal.h>
#include <Maya/MTime.h>
#include <Maya/MString.h>

namespace doodle::motion::kernel {

class Screenshot {
 private:
  FSys::path p_file;
  uint32_t p_width;
  uint32_t p_height;

  std::unique_ptr<MayaRenderOpenGL> p_view;

 public:
  Screenshot(FSys::path path);
  void save();
};

}  // namespace doodle::motion::kernel