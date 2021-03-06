#include <DoodleLib/FileWarp/MayaFile.h>
#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/core/coreset.h>
#include <DoodleLib/threadPool/ThreadPool.h>

#include <boost/process.hpp>

#include <boost/format.hpp>

#include <Logger/Logger.h>

DOODLE_NAMESPACE_S
MayaFile::MayaFile(FSys::path mayaPath)
    : LongTerm(),
      p_path(std::move(mayaPath)) {
  if (!FSys::exists(p_path) && coreSet::getSet().hasMaya())
    p_path = coreSet::getSet().MayaPath();
  else
    throw DoodleError{"无法找到maya启动器"};
}

FSys::path MayaFile::createTmpFile() const {
  //开始写入临时文件

  const static auto tmp_path = coreSet::getSet().getCacheRoot("maya");
  auto k_tmp_path            = tmp_path / (boost::uuids::to_string(coreSet::getSet().getUUID()) + ".py");
  auto k_file_py             = cmrc::DoodleLibResource::get_filesystem().open("resource/mayaExport.py");

  {  //写入文件后直接关闭
    FSys::fstream file{k_tmp_path, std::ios::out | std::ios::binary};
    file.write(k_file_py.begin(), k_file_py.size());
  }
  return k_tmp_path;
}

bool MayaFile::exportFbxFile(const FSys::path& file_path, const FSys::path& export_path) const {
  auto k_export_path = file_path.parent_path() / file_path.stem();
  if (!export_path.empty()) k_export_path = export_path;

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  if (!FSys::exists(k_export_path))
    FSys::create_directories(k_export_path);

  if (!FSys::exists(file_path))
    return false;
  auto k_tmp_path = this->createTmpFile();
  //生成命令
  boost::wformat str{LR"("%4%/mayapy.exe" %1% --path %2% --exportpath %3%)"};
  str % k_tmp_path.generic_wstring() % file_path.generic_wstring() % k_export_path.generic_wstring();
  str % p_path.generic_wstring();
  // boost::format str{R"(%1% --path %2% --exportpath %3%)"};
  // str % k_tmp_path.generic_string() % file_path.generic_string() % k_export_path.generic_string();

  DOODLE_LOG_INFO(str.str())

  STARTUPINFO si{};
  PROCESS_INFORMATION pi{};
  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));

  try {
    //使用windowsIPA创建子进程
    CreateProcess(
        nullptr,
        (wchar_t*)str.str().c_str(),
        nullptr,
        nullptr,
        false,
        CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,  //CREATE_NEW_CONSOLE
        nullptr,
        p_path.generic_wstring().c_str(),  //R"(C:\Program Files\Autodesk\Maya2018\bin\)"
        &si,
        &pi);
    // boost::process::system(command.c_str(), env);
  } catch (const std::runtime_error& err) {
    DOODLE_LOG_WARN(err.what())
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return false;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  // boost::process::ipstream k_in{};
  // boost::process::system(str.str());
  // boost::process::child k_c{str.str(), boost::process::windows::hide};
  // boost::process::child k_c{boost::process::cmd = str.str(), boost::process::std_out > k_in, boost::process::windows::hide};

  // auto str_r = std::string{};
  // while (k_c.running() && std::getline(k_in, str_r) && !str_r.empty()) {
  //   // std::cout << str_r << std::endl;
  //   DOODLE_LOG_INFO(str_r);
  // }

  // k_c.wait();
  FSys::remove(k_tmp_path);
  FSys::copy_file(file_path, k_export_path / file_path.filename(), FSys::copy_options::overwrite_existing);
  return true;
}

bool MayaFile::batchExportFbxFile(const std::vector<FSys::path>& file_path) const {
  auto thread_pool = ThreadPool{std::thread::hardware_concurrency()};
  auto result      = std::map<FSys::path, std::future<bool>>{};
  for (auto&& file : file_path) {
    if (file.extension() == ".ma" || file.extension() == ".mb") {
      result.insert(
          {file,
           thread_pool
               .enqueue(
                   [this, file]() -> bool {
                     return this->exportFbxFile(file);
                   })});
    }
  }
  std::future_status status{};
  auto it    = result.begin();
  auto size  = boost::numeric_cast<float>(result.size());
  auto k_pro = float{0};
  auto mess  = boost::format{"文件:%s-->%s\n"};

  while (!result.empty()) {
    status = it->second.wait_for(std::chrono::milliseconds{10});
    if (status == std::future_status::ready) {
      //成功就加一
      ++k_pro;
      //添加进度
      this->progress(boost::numeric_cast<int>((k_pro / size) * 100));
      // try {
      // } catch (const DoodleError& err) {
      //   //添加错误
      //   DOODLE_LOG_WARN(err.what());
      // }
      mess % it->first % ((it->second.get()) ? "成功" : "失败");
      //发送消息
      this->messagResult(mess.str());
      mess.clear();
      //擦除容器内数据
      it = result.erase(it);
    } else {
      //等待超时后继续迭代
      ++it;
    }
    if (it == result.end()) {
      it = result.begin();
    }
  }
  this->finished();
  return true;
}

bool MayaFile::checkFile() {
  return true;
}

DOODLE_NAMESPACE_E
