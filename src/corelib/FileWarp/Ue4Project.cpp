#include <corelib/FileWarp/Ue4Project.h>
#include <corelib/Exception/Exception.h>
#include <corelib/libWarp/WinReg.hpp>
namespace doodle {
Ue4Project::Ue4Project(FSys::path project_path)
    : p_ue_path(),
      p_ue_Project_path(std::move(project_path)) {
  auto key     = winreg::RegKey{HKEY_LOCAL_MACHINE, LR"(SOFTWARE\EpicGames\Unreal Engine)"};
  auto subKeys = key.EnumSubKeys();
}

void Ue4Project::createShotFolder(const int start, const int end) {
}

}  // namespace doodle