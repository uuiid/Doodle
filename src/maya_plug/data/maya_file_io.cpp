//
// Created by TD on 2021/12/6.
//

#include "maya_file_io.h"

#include <maya_plug/data/reference_file.h>
#include <maya_plug/main/maya_plug_fwd.h>

// #include <Windows.h>
#include "maya_conv_str.h"
#include <maya/MFileIO.h>
#include <maya/MFileObject.h>
//
// #ifdef WIN32
// #undef WIN32
// #endif
//
// #define WIN32
#include <maya/adskDataAssociations.h>
#include <maya/adskDataStream.h>
#include <maya/adskDebugPrint.h>
namespace doodle::maya_plug {

FSys::path maya_file_io::get_current_path() {
  auto k_s = MFileIO::currentFile();
  return {k_s.asUTF8()};
}
FSys::path maya_file_io::work_path(const FSys::path& in_path) {
  MFileObject k_obj{};

  k_obj.setRawName(d_str{in_path.generic_string()});
  k_obj.setResolveMethod(MFileObject::MFileResolveMethod::kNone);
  return k_obj.resolvedFullName().asUTF8();
}

bool maya_file_io::save_file(const FSys::path& in_file_path) {
  MStatus k_s{};
  if (!exists(in_file_path.parent_path())) {
    create_directories(in_file_path.parent_path());
  }
  auto l_ext = in_file_path.extension().generic_string();
  MString l_string{};
  if (l_ext == ".ma") {
    l_string.setUTF8("mayaAscii");
  } else if (l_ext == ".mb") {
    l_string.setUTF8("mayaBinary");
  }
  k_s = MFileIO::resetError();
  DOODLE_MAYA_CHICK(k_s);

  k_s = MFileIO::saveAs(d_str{in_file_path.generic_string()}, l_string.asChar(), true);
  DOODLE_MAYA_CHICK(k_s);
  return false;
}
bool maya_file_io::upload_file(const FSys::path& in_source_path, const FSys::path& in_prefix) {
  DOODLE_CHICK(FSys::is_regular_file(in_source_path), doodle_error{"{} 路径不存在或者不是文件"});
  bool result{false};

  auto l_upload_path = g_reg()->ctx().get<project_config::base_config>().get_upload_path();
  l_upload_path /= in_prefix;
  l_upload_path /= maya_file_io::get_current_path().stem();
  if (!FSys::exists(l_upload_path)) FSys::create_directories(l_upload_path);

  auto l_target = l_upload_path / in_source_path.filename();
  try {
    DOODLE_LOG_INFO("开始备份文件 {}", l_target);
    FSys::backup_file(l_target);
  } catch (const FSys::filesystem_error& error) {
    DOODLE_LOG_ERROR("备份文件失败, {}", boost::diagnostic_information(error));
  }

  try {
    DOODLE_LOG_INFO("开始复制文件 {} -> {}", in_source_path, l_upload_path);
    FSys::copy_file(in_source_path, l_target, FSys::copy_options::overwrite_existing);
    result = true;
  } catch (const FSys::filesystem_error& error) {
    DOODLE_LOG_ERROR("复制文件失败, {}", boost::diagnostic_information(error));
  }
  return result;
}

void maya_file_io::set_workspace(const FSys::path& in_path) {
  FSys::path l_path{in_path.parent_path()};
  if (!FSys::exists(l_path / "workspace.mel")) {
    l_path = l_path.parent_path();
  }

  if (!FSys::exists(l_path / "workspace.mel")) {
    std::string const l_s{fmt::format(
        R"(workspace -baseWorkspace "default" -openWorkspace "{}")",
        (in_path.parent_path().filename() == "ma" ? l_path : in_path.parent_path()).generic_string()
    )};

    maya_chick(MGlobal::executeCommand(d_str{l_s}));
    maya_chick(MGlobal::executeCommand(R"(workspace -saveWorkspace)"));
  } else {
    std::string const l_s{fmt::format(R"(workspace -openWorkspace "{}")", l_path.generic_string())};
    maya_chick(MGlobal::executeCommand(d_str{l_s}));
  }
}

auto open_file_impl(const MString& in_str, MFileIO::ReferenceMode in_mod) {
  MStatus l_s{};
  try {
    l_s = MFileIO::open(in_str, nullptr, true, in_mod, true);
  } catch (...) {
    log_error(boost::current_exception_diagnostic_information());
  }
  return l_s;
}

void maya_file_io::open_file(const FSys::path& in_file_path, MFileIO::ReferenceMode in_mode) {
  auto l_str = conv::to_ms(in_file_path.generic_string());
  try {
    maya_chick(open_file_impl(l_str, in_mode));
  } catch (const std::exception& e) {
    default_logger_raw()->log(log_loc(), level::err, "打开文件失败, 无法保证输出正确:{} {}", e.what(), in_file_path);
  }
}

}  // namespace doodle::maya_plug
