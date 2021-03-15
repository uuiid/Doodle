#include <lib/kernel/Maya/MayaVideo.h>

#include <lib/kernel/Exception.h>
#include <lib/kernel/Maya/MayaRenderOpenGL.h>
#include <lib/kernel/ExeWarp/FFmpegWarp.h>

#include <iostream>
#include <sstream>

#include <Maya/MGlobal.h>
#include <Maya/MAnimControl.h>
#include <Maya/MFileIO.h>

namespace doodle::motion::kernel {
MayaVideo::MayaVideo(FSys::path file)
    : p_file(std::move(file)),
      p_file_image(),
      p_view(std::make_unique<MayaRenderOpenGL>(1280, 720)),
      p_ffmpeg(std::make_unique<FFmpegWarp>()) {
}

void MayaVideo::save() {
  auto p_path = FSys::temp_directory_path() / "doodle";
  // auto p_path = FSys::path{"D:/tmp"} / "doodle";
  if (!FSys::exists(p_path))
    FSys::create_directories(p_path);

  p_view->getFileName.connect([=](const MTime& time) -> MString {
    MString fileName{};
    MString k_tmp{};
    k_tmp.setUTF8(p_path.generic_u8string().c_str());
    fileName += k_tmp;
    fileName += "/";

    //添加uuid名称
    k_tmp.setUTF8(this->p_file.stem().generic_u8string().c_str());
    fileName += k_tmp;
    fileName += ".";

    //添加序列号
    std::stringstream str{};
    str << std::setw(5) << std::setfill('0') << (time.value());
    fileName += str.str().c_str();

    fileName += ".png";
    this->p_file_image.emplace_back(std::make_shared<FSys::path>(fileName.asWChar()));
    return fileName;
  });
  p_view->save(MAnimControl::animationStartTime(), MAnimControl::animationEndTime());

  for (auto&& it : p_file_image)
    if (!FSys::exists(*it))
      throw MayaError("not create file " + it->generic_u8string());

  p_ffmpeg->imageToVideo(p_file_image, p_file, "doodle");
  if (!FSys::exists(p_file)) throw NotFileError("nor create file" + p_file.generic_u8string());
}

MayaVideo::~MayaVideo() {
  for (auto&& it : p_file_image) {
    FSys::remove(*it);
  }
}
}  // namespace doodle::motion::kernel