#include <doodle_core/core/core_set.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/toolkit/toolkit.h>

#include <winreg/WinReg.hpp>
#if defined(_WIN32)

#include <ShlObj.h>
#endif

namespace doodle {

void toolkit::installMayaPath() {
  try {
    auto mayadoc = win::get_pwd();
    mayadoc /= "maya";
    mayadoc /= "modules";
    mayadoc /= "doodle";

    auto sourePath = register_file_type::program_location().parent_path();
    sourePath /= "maya";

    if (!FSys::exists(mayadoc)) {
      FSys::create_directories(mayadoc);
    } else
      FSys::remove_all(mayadoc);

    DOODLE_LOG_INFO(fmt::format("install plug : {} --> {}", sourePath, mayadoc));
    copy(sourePath, mayadoc, FSys::copy_options::recursive | FSys::copy_options::update_existing);

    static std::string const k_mod{R"(+ doodle 1.1 .\doodle
MYMODULE_LOCATION:= .
PATH+:= plug-ins
PYTHONPATH+:= scripts
)"};
    {
      auto k_p = mayadoc.parent_path() / "doodle.mod";
      DOODLE_LOG_INFO("写入 {}", k_p);
      FSys::ofstream k_file{k_p};
      k_file << k_mod;
    }
  } catch (FSys::filesystem_error &err) {
    DOODLE_LOG_ERROR(boost::diagnostic_information(err));
    throw;
  }
}

void install_SideFX_Labs(const FSys::path &path) {
  if (FSys::exists(path)) {
    FSys::remove_all(path);
  } else {
    FSys::create_directories(path);
  }

  auto sourePath = register_file_type::program_location().parent_path() / "SideFX_Labs";
  DOODLE_LOG_INFO(fmt::format("install plug : {} --> {}", sourePath, path));
  copy(sourePath, path, FSys::copy_options::recursive | FSys::copy_options::update_existing);
}

void install_UnrealEngine5VLC(const FSys::path &path) {
  if (FSys::exists(path)) {
    FSys::remove_all(path);
  } else {
    FSys::create_directories(path);
  }

  auto sourePath = register_file_type::program_location().parent_path() / "UnrealEngine5VLC";
  DOODLE_LOG_INFO(fmt::format("install plug : {} --> {}", sourePath, path));
  copy(sourePath, path, FSys::copy_options::recursive | FSys::copy_options::update_existing);
}

void toolkit::installUePath(const FSys::path &path) {
  try {
    /// \brief 安装我们自己的插件
    auto &set      = core_set::get_set();
    auto sourePath = register_file_type::program_location().parent_path();
    auto l_name{set.ue4_version};
    if (auto l_f = l_name.find('.'); l_f != std::string::npos) {
      l_name.erase(l_f, 1);
    }
    sourePath /= fmt::format("ue{}_Plug", l_name);
    auto targetPath = path / "Plugins" / "Doodle";

    if (FSys::exists(targetPath)) {
      FSys::remove_all(targetPath);
    } else {
      FSys::create_directories(targetPath);
    }

    DOODLE_LOG_INFO(fmt::format("install plug : {} --> {}", sourePath, targetPath));
    copy(sourePath, targetPath, FSys::copy_options::recursive | FSys::copy_options::update_existing);
    /// \brief 安装houdini labs 插件
    install_SideFX_Labs(targetPath.parent_path() / "SideFX_Labs");
    install_UnrealEngine5VLC(targetPath.parent_path() / "UnrealEngine5VLC");
  } catch (FSys::filesystem_error &error) {
    DOODLE_LOG_ERROR(boost::diagnostic_information(error));
    throw;
  }
}

void toolkit::modifyUeCachePath() {
  // winreg::RegKey l_key{};
  // l_key.Create(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Session Manager\\Environment");
  // l_key.SetStringValue(L"UE-LocalDataCachePath", L"%GAMEDIR%DerivedDataCache");
  // l_key.SetStringValue(
  //     L"UE-SharedDataCachePath",
  //     fmt::format(L"{}\\UE\\DerivedDataCache", conv::utf_to_utf<wchar_t>(core_set::get_set().depot_ip))
  // );
  // constexpr static const auto *L_Param = L"Environment";
  // ::SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(L_Param), SMTO_BLOCK, 100,
  // NULL);

  auto l_path = core_set::get_set().ue4_path / "Engine" / "Config" / "BaseEngine.ini";
  if (!FSys::is_regular_file(l_path)) return;
  std::string l_file{};
  {
    FSys::ifstream l_f{l_path};
    l_file = {std::istreambuf_iterator<char>(l_f), std::istreambuf_iterator<char>()};
  }
  FSys::backup_file(l_path);
  boost::replace_all(l_file, "ENGINEVERSIONAGNOSTICUSERDIR", "GAMEDIR");
  boost::replace_all(
      l_file, "/ACLPlugin/ACLAnimBoneCompressionSettings", "/Engine/Animation/DefaultRecorderBoneCompression"
  );
  {
    FSys::ofstream l_f{l_path};
    l_f << l_file;
  }
}

bool toolkit::deleteUeCache() {
#if defined(_WIN32)
  // 这里我们手动做一些工作
  // 获取环境变量
  PWSTR pManager;
  SHGetKnownFolderPath(FOLDERID_LocalAppData, NULL, nullptr, &pManager);
  DOODLE_CHICK(pManager, doodle_error{"无法找到保存路径"});

  FSys::path path{pManager};
  CoTaskMemFree(pManager);

  path /= "UnrealEngine";
  DOODLE_LOG_INFO(fmt::format("delete Folder : {}", path));

  //  FSys::remove_all(path);

  for (FSys::recursive_directory_iterator it{path}; it != FSys::recursive_directory_iterator{}; it++) {
    DOODLE_LOG_INFO(fmt::format("delete : {}", it->path()));
    if (it->is_regular_file()) FSys::remove(*it);
  }

#endif
  return true;
}
void toolkit::install_houdini_plug() {
  try {
    {
      auto l_doc = win::get_pwd();
      l_doc /= "houdini19.0/plus/doodle";
      auto l_source = register_file_type::program_location().parent_path() / "houdini";
      if (FSys::exists(l_doc)) {
        FSys::remove_all(l_doc);
      } else {
        FSys::create_directories(l_doc);
      }
      DOODLE_LOG_INFO(fmt::format("install plug : {} --> {}", l_source, l_doc));
      FSys::copy(l_source, l_doc, FSys::copy_options::recursive | FSys::copy_options::update_existing);
    }
    {
      auto l_doc = win::get_pwd();
      l_doc /= "houdini19.0/packages/doodle_houdini.json";
      auto l_source = register_file_type::program_location().parent_path() / "houdini" / "doodle_houdini.json";
      if (FSys::exists(l_doc)) {
        FSys::remove_all(l_doc);
      } else {
        FSys::create_directories(l_doc.parent_path());
      }
      DOODLE_LOG_INFO(fmt::format("install plug : {} --> {}", l_source, l_doc));
      FSys::copy(l_source, l_doc, FSys::copy_options::recursive | FSys::copy_options::update_existing);
    }

  } catch (FSys::filesystem_error &error) {
    DOODLE_LOG_ERROR(boost::diagnostic_information(error));
    throw;
  }
}

}  // namespace doodle
