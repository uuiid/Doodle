#include <lib/kernel/ExeWarp/FFmpegWarp.h>

#include <lib/kernel/BoostUuidWarp.h>

#include <iostream>
#include <fstream>

#include <boost/format.hpp>
#include <boost/process.hpp>

#include <Maya/MGlobal.h>
namespace doodle::motion::kernel {

FFmpegWarp::FFmpegWarp()
    : p_file(FSys::temp_directory_path() /
             (boost::uuids::to_string(boost::uuids::random_generator{}()) + ".txt")) {
}
FFmpegWarp::~FFmpegWarp() {
  FSys::remove(p_file);
}
bool FFmpegWarp::imageToVideo(const std::vector<std::shared_ptr<FSys::path>> &imagepaths,
                              const FSys::path &videoPath,
                              const std::string &subtitles) const {
  std::fstream file{p_file, std::ios::out | std::ios::binary};

  for (auto &&item : imagepaths) {
    file << boost::format("file '%1%'\n") % item->generic_u8string();
  }
  file.close();

  if (!FSys::exists(videoPath.parent_path()))
    FSys::create_directories(videoPath.parent_path());
  if (FSys::exists(videoPath))
    FSys::remove(videoPath);

  auto com_arg = boost::format(R"(ffmpeg.exe
 -r 25 -f concat -safe 0 -i %1% 
 -filter_complex "drawtext=text='%2%':fontcolor=0xc62d1d:fontsize=44:x=10:y=10:shadowx=3:shadowy=3" 
 -c:v libx264 -pix_fmt yuv420p -s 1920*1080 %3%)");
  com_arg % p_file.generic_u8string() % subtitles % videoPath;

  if (runFFmpeg(com_arg.str()))
    return FSys::exists(videoPath);
  else
    return false;
}

bool FFmpegWarp::runFFmpeg(const std::string &command) const {
  // auto k_ffmpeg_path = boost::process::search_path("ffmpeg");
  try {
    boost::process::ipstream stream{};
    boost::process::system(command.c_str(),
                           boost::process::std_out > stream,
                           boost::process::std_err > stream,
                           boost::process::std_in < boost::process::null);
    std::string line{};
    MString str{};
    while (stream && std::getline(stream, line) && !line.empty()) {
      str.setUTF8(line.c_str());
      MGlobal::displayInfo(str);
    }
  } catch (const std::exception &e) {
    MGlobal::displayError(e.what());
    return false;
  }
  return true;
}
}  // namespace doodle::motion::kernel