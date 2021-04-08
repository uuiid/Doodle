#pragma once

#include <lib/MotionGlobal.h>

//maya api
#include <Maya/MGlobal.h>
#include <Maya/MTime.h>
#include <Maya/MString.h>

#include <boost/signals2.hpp>

namespace doodle::motion::kernel {
class MayaRenderOpenGL {
 private:
  MTime p_start_pos;
  MTime p_end_pos;
  MTime p_current_time;
  uint32_t p_width;
  uint32_t p_height;

  MString p_post_render_notification_name;
  MString p_post_render_notification_semantic;
  static void captureCallback(MHWRender::MDrawContext& context, void* clientData);

 public:
  MayaRenderOpenGL(uint32_t width, uint32_t height);
  ~MayaRenderOpenGL();
  void save(const MTime& start, const MTime& end);

  boost::signals2::signal<MString(const MTime&)> getFileName;
};

}  // namespace doodle::motion::kernel