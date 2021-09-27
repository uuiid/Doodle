#include <DoodleLib/core/ue4_setting.h>
#include <DoodleLib/libWarp/boost_locale_warp.h>
#include <Exception/exception.h>

#include <libWarp/WinReg.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(doodle::ue4_setting)
namespace doodle {
ue4_setting::ue4_setting()
    : ue4_path(),
      ue4_version("4.25"),
      shot_start(0),
      shot_end(100) {
}

ue4_setting& ue4_setting::Get() {
  static ue4_setting install;
  return install;
}

const std::string& ue4_setting::Version() const noexcept {
  return ue4_version;
}

void ue4_setting::setVersion(const std::string& Version) noexcept {
  ue4_version = Version;
}

bool ue4_setting::hasPath() const {
  return !ue4_path.empty();
}

const FSys::path& ue4_setting::Path() const noexcept {
  return ue4_path;
}

void ue4_setting::setPath(const FSys::path& Path) noexcept {
  ue4_path = Path;
}

const std::int32_t& ue4_setting::ShotStart() const noexcept {
  return shot_start;
}

void ue4_setting::setShotStart(const std::int32_t& ShotStart) noexcept {
  shot_start = ShotStart;
}

const std::int32_t& ue4_setting::ShotEnd() const noexcept {
  return shot_end;
}

void ue4_setting::setShotEnd(const std::int32_t& ShotEnd) noexcept {
  shot_end = ShotEnd;
}

void ue4_setting::testValue() {
  if (shot_end <= shot_start) {
    throw doodle_error{"结束镜头小于开始镜头!"};
  }
#ifdef _WIN32
  if (ue4_path.empty()) {
    auto key_str = conv::utf_to_utf<wchar_t>(fmt::format(R"(SOFTWARE\EpicGames\Unreal Engine\{})", ue4_setting::Get().Version()));
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
