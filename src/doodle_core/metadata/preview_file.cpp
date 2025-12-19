#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/preview_file.h>

#include <filesystem>
#include <set>
#include <string>

namespace doodle {

bool preview_file::is_image() {
  const std::set<std::string> image_extensions = {"png", "jpg", "jpeg", "bmp", ".png", ".jpg", ".jpeg", ".bmp"};

  return image_extensions.find(extension_) != image_extensions.end();
}
bool preview_file::is_video() {
  const std::set<std::string> video_extensions = {"mp4", "mov", "avi", "mkv", ".mp4", ".mov", ".avi", ".mkv"};
  return video_extensions.find(extension_) != video_extensions.end();
}

}  // namespace doodle