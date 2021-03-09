#pragma once

#include <lib/MotionGlobal.h>

//maya api
#include <Maya/MGlobal.h>
#include <Maya/MTime.h>
#include <Maya/MString.h>

namespace doodle::motion::kernel {
class MayaVideo;

class Screenshot {
 private:
  FSys::path p_file;

  MTime p_start_pos;
  MTime p_end_pos;
  MTime p_current_time;
  uint32_t p_width;
  uint32_t p_height;

  MString p_post_render_notification_name;
  MString p_post_render_notification_semantic;

  std::unique_ptr<MayaVideo> p_video;

  static void captureCallback(MHWRender::MDrawContext& context, void* clientData);
  void DebugPring(MHWRender::MDrawContext& context);

 public:
  Screenshot(FSys::path path);
  void save(const MTime& start, const MTime& end);
  ~Screenshot();
};

}  // namespace doodle::motion::kernel