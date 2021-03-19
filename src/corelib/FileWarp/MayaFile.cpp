#include <corelib/FileWarp/MayaFile.h>
#include <corelib/Exception/Exception.h>
#include <corelib/core/coreset.h>

#include <boost/process.hpp>
#include <boost/format.hpp>
#include <corelib/libWarp/BoostUuidWarp.h>
#include <CoreResourceMayaExportFbx.h>
#include <loggerlib/Logger.h>

DOODLE_NAMESPACE_S
MayaFile::MayaFile(FSys::path mayaPath)
    : p_path(std::move(mayaPath)) {
  findMaya();
}

void MayaFile::findMaya() {
  if (FSys::exists(p_path))
    return;

  if (FSys::exists(R"(C:\Program Files\Autodesk\Maya2020\bin)")) {
    p_path = R"(C:\Program Files\Autodesk\Maya2020\bin\)";
  } else if (FSys::exists(R"(C:\Program Files\Autodesk\Maya2019\bin)")) {
    p_path = R"(C:\Program Files\Autodesk\Maya2019\bin\)";
  } else if (FSys::exists(R"(C:\Program Files\Autodesk\Maya2018\bin)")) {
    p_path = R"(C:\Program Files\Autodesk\Maya2018\bin\)";
  } else {
    throw std::runtime_error("无法找到maya.exe");
  }
  return;
}

const FSys::path MayaFile::createTmpFile() const {
  //开始写入临时文件
  const static auto tmp_path = FSys::temp_directory_path();
  auto k_tmp_path            = tmp_path / (boost::uuids::to_string(coreSet::getSet().getUUID()) + ".py");
  auto& k_file_py            = bin2cpp::getCoreResourceMayaExportFbxFile();

  {  //写入文件后直接关闭
    std::fstream file{k_tmp_path, std::ios::out | std::ios::binary};
    file.write(k_file_py.getBuffer(), k_file_py.getSize());
  }
  return k_tmp_path;
}

bool MayaFile::exportFbxFile(const FSys::path& file_path, const FSys::path& export_path) const {
  auto k_export_path = file_path.parent_path() / file_path.stem();
  if (!export_path.empty()) k_export_path = export_path;

  if (!FSys::exists(k_export_path))
    FSys::create_directories(k_export_path);

  if (!FSys::exists(file_path))
    return false;
  auto k_tmp_path = this->createTmpFile();
  //生成命令
  boost::format str{"mayapy.exe %1% --path %2% --exportpath %3%"};
  str % k_tmp_path.generic_string() % file_path.generic_string() % k_export_path.generic_string();

  boost::process::ipstream k_in{};
  auto k_env_                       = boost::this_process::environment();
  boost::process::environment k_env = k_env_;
  k_env["PATH"] += p_path.generic_string();
  boost::process::child k_c{str.str(), boost::process::std_out > k_in, k_env};

  auto str_r = std::string{};
  while (k_c.running() && std::getline(k_in, str_r) && !str_r.empty()) {
    // std::cout << str_r << std::endl;
    DOODLE_LOG_INFO(str_r);
  }

  k_c.wait();
  FSys::remove(k_tmp_path);
  return true;
}

bool MayaFile::checkFile() {
  return true;
}

DOODLE_NAMESPACE_E