#include <lib/kernel/ExeWarp/FFmpegWarp.h>

#include <lib/kernel/BoostUuidWarp.h>
#include <lib/kernel/Exception.h>
#include <iostream>
#include <fstream>

#include <boost/format.hpp>
#include <boost/process.hpp>
#include <boost/dll/library_info.hpp>
#include <boost/dll.hpp>

#include <Maya/MGlobal.h>
namespace doodle::motion::kernel {

FFmpegWarp::FFmpegWarp()
    : p_file(FSys::temp_directory_path() / "doodle" /
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
    file << boost::format("file '%1%'\n") % item->filename().generic_u8string();
  }
  file.close();

  if (!FSys::exists(videoPath.parent_path()))
    FSys::create_directories(videoPath.parent_path());
  if (FSys::exists(videoPath))
    FSys::remove(videoPath);

  auto com_arg = boost::format(
      "ffmpeg.exe"
      " -r 25 -f concat -safe 0 -i %1% "
      " -filter_complex \"drawtext=text='%2%':fontcolor=0xc62d1d:fontsize=44:x=10:y=10:shadowx=3:shadowy=3\""
      " -c:v libx264 -pix_fmt yuv420p -s 1920*1080 %3%");
  com_arg % p_file.generic_u8string() % subtitles % videoPath.generic_u8string();

  if (runFFmpeg(com_arg.str()))
    return FSys::exists(videoPath);
  else
    return false;
}

bool FFmpegWarp::runFFmpeg(const std::string &command) const {
  MGlobal::displayInfo(command.c_str());

  // auto k_env = boost::this_process::environment();
  // if (k_ffmpeg_path.empty())
  // boost::process::environment k_env_ = k_env;
  // k_env_[L"PATH"] += LR"(C:\Program Files\ffmpeg\bin)";
  auto k_dll = boost::dll::this_line_location().parent_path();
  // k_env_["PATH"] += k_dll.generic_string();
  // k_env_["PATH"] += R"(C:\Windows\Fonts\)";
  // auto k_ffmpeg_path = boost::process::search_path("ffmpeg");
  // if (k_ffmpeg_path.empty())
  //   throw FFmpegError("无法找到 ffmpeg");
  auto k_dll_str = k_dll.generic_string();
  k_dll_str.append("/bin/").append(command);
  try {
    boost::process::ipstream stream{};

    boost::process::child child{k_dll_str,
                                boost::process::std_out > stream,
                                boost::process::std_err > stream};
    MString str{};
    std::string line;

    while (child.running() && std::getline(stream, line) && !line.empty()) {
      str.setUTF8(line.c_str());
      MGlobal::displayInfo(str);
    }
    child.wait();

  } catch (const std::exception &e) {
    MGlobal::displayError(e.what());
    return false;
  }
  return true;
}
}  // namespace doodle::motion::kernel