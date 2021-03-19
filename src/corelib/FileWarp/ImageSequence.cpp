#include <corelib/FileWarp/ImageSequence.h>
namespace doodle {
ImageSequence::ImageSequence(decltype(p_paths) paths)
    : p_paths(std::move(paths)) {
}

void ImageSequence::createVideoFile(const FSys::path &out_file) {
}

}  // namespace doodle