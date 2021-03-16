#include <lib/kernel/Maya/Screenshot.h>

#include <lib/kernel/Exception.h>
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
  auto k_path = FSys::temp_directory_path() / "doodle" / p_file.filename();

  p_view->getFileName.connect([k_path](const MTime& time) -> MString {
    MString fileName{};
    // MString k_tmp{};
    // k_tmp.setUTF8(this->p_file.parent_path().generic_u8string().c_str());
    // fileName += k_tmp;
    // fileName += "/";
    // k_tmp.setUTF8(this->p_file.stem().generic_u8string().c_str());
    // fileName += k_tmp;
    // fileName += ".";
    // std::stringstream str{};
    // str << std::setw(5) << std::setfill('0') << (time.value());
    // fileName += str.str().c_str();

    // k_tmp.setUTF8(this->p_file.generic_u8string().c_str());
    fileName.setUTF8(k_path.generic_u8string().c_str());
    return fileName;
  });
  p_view->save(MAnimControl::currentTime(), MAnimControl::currentTime());
  if (!FSys::exists(k_path)) throw NotFileError(k_path);

  FSys::copy(k_path, p_file);
  if (!FSys::exists(p_file)) throw NotFileError(p_file);

  FSys::remove(k_path);
}

}  // namespace doodle::motion::kernel

#undef DOODLE_MAYA_CHICK