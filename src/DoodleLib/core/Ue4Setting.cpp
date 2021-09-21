#include <DoodleLib/core/Ue4Setting.h>
#include <DoodleLib/libWarp/boost_locale_warp.h>
#include <Exception/Exception.h>

#include <libWarp/WinReg.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(doodle::Ue4Setting)
namespace doodle {
Ue4Setting::Ue4Setting()
    : ue4_path(),
      ue4_version("4.25"),
      shot_start(0),
      shot_end(100) {
}

Ue4Setting& Ue4Setting::Get() {
  static Ue4Setting install;
  return install;
}

const std::string& Ue4Setting::Version() const noexcept {
  return ue4_version;
}

void Ue4Setting::setVersion(const std::string& Version) noexcept {
  ue4_version = Version;
}

bool Ue4Setting::hasPath() const {
  return !ue4_path.empty();
}

const FSys::path& Ue4Setting::Path() const noexcept {
  return ue4_path;
}

void Ue4Setting::setPath(const FSys::path& Path) noexcept {
  ue4_path = Path;
}

const std::int32_t& Ue4Setting::ShotStart() const noexcept {
  return shot_start;
}

void Ue4Setting::setShotStart(const std::int32_t& ShotStart) noexcept {
  shot_start = ShotStart;
}

const std::int32_t& Ue4Setting::ShotEnd() const noexcept {
  return shot_end;
}

void Ue4Setting::setShotEnd(const std::int32_t& ShotEnd) noexcept {
  shot_end = ShotEnd;
}

void Ue4Setting::testValue() {
  if (shot_end <= shot_start) {
    throw DoodleError{"结束镜头小于开始镜头!"};
  }
#ifdef _WIN32
  if (ue4_path.empty()) {
    auto key_str = conv::utf_to_utf<wchar_t>(fmt::format(R"(SOFTWARE\EpicGames\Unreal Engine\{})", Ue4Setting::Get().Version()));
    try {
      auto key = winreg::RegKey{HKEY_LOCAL_MACHINE};
      key.Open(HKEY_LOCAL_MACHINE, key_str, KEY_QUERY_VALUE);
      ue4_path = FSys::path{key.GetStringValue(L"InstalledDirectory")};

    } catch (const winreg::RegException& e) {
      DOODLE_LOG_WARN(e.what());
    }
  }
#endif
}

}  // namespace doodle
