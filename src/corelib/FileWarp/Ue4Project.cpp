#include <corelib/FileWarp/Ue4Project.h>
#include <corelib/Exception/Exception.h>
#include <corelib/libWarp/WinReg.hpp>
#include <corelib/core/Ue4Setting.h>

#include <boost/format.hpp>
#include <boost/locale.hpp>
namespace doodle {
Ue4Project::Ue4Project(FSys::path project_path)
    : p_ue_path(),
      p_ue_Project_path(std::move(project_path)) {
  auto& ue = Ue4Setting::Get();
  if (ue.hasPath()) {
    p_ue_path = ue.Path() / "Engine/Binaries/Win64/UE4Editor.exe";
  } else {
    auto key_str = boost::wformat{LR"(SOFTWARE\EpicGames\Unreal Engine\%s)"};
    auto wv      = boost::locale::conv::utf_to_utf<wchar_t>(Ue4Setting::Get().Version());
    key_str % wv;

    auto key  = winreg::RegKey{HKEY_LOCAL_MACHINE, key_str.str()};
    p_ue_path = FSys::path{key.GetStringValue(L"InstalledDirectory")} / "Engine/Binaries/Win64/UE4Editor.exe";
  }
}

void Ue4Project::createShotFolder(const int start, const int end) {
}

}  // namespace doodle