// #include

#pragma once

#define DOODLE_DISABLE_COPY(className)   \
  className(const className &) = delete; \
  className &operator=(const className &) = delete;

namespace doodle {
namespace FSys = boost::filesystem;
using variant  = std::variant<
    std::string,  //
    int64_t       //
    >;

namespace motion {
namespace ui {

class TreeDirItem;
using TreeDirItemPtr = std::shared_ptr<TreeDirItem>;

}  // namespace ui
namespace kernel {
class MayaRenderOpenGL;
class FFmpegWarp;

class PlayerMotion;
using PlayerMotionPtr = std::shared_ptr<PlayerMotion>;
class MotionFile;
using MotionFilePtr = std::shared_ptr<MotionFile>;
}  // namespace kernel
}  // namespace motion
}  // namespace doodle