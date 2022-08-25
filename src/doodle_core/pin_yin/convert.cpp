#include <doodle_core/pin_yin/convert.h>

#include <boost/locale.hpp>
#include <regex>

namespace doodle {
static std::vector<std::string> pinyin_list{};
convert::convert() {
  const auto &resource = cmrc::DoodleLibResource::get_filesystem().open("resource/zhtopy.txt");

  std::string ZhongWenToPinYin{resource.begin(), resource.size()};
  std::regex regex{R"(\s)"};
  auto iter = std::sregex_token_iterator(
      ZhongWenToPinYin.begin(), ZhongWenToPinYin.end(), regex, -1
  );
  while (iter != std::sregex_token_iterator{}) {
    pinyin_list.push_back(*iter);
    ++iter;
  }
}

convert::~convert() = default;

std::string convert::toEn(const std::string &conStr) {
  if (conStr.empty()) return {};
  auto datas = boost::locale::conv::utf_to_utf<wchar_t>(conStr);
  std::string result{};
  for (auto data : datas) {
    if (data >= 0x4e00 && data <= 0x9fa5) {
      result.append(pinyin_list[data - 0x4e00]);
    } else {
      result.append(boost::locale::conv::utf_to_utf<char>(std::wstring{data}));
    }
  }
  // DOODLE_LOG_INFO(fmt::format("{} to {}", conStr, result));
  return result;
}

std::string convert::toEn(const std::wstring &conStr) {
  if (conStr.empty()) return {};

  std::string result{};
  for (auto data : conStr) {
    if (data >= 0x4e00 && data <= 0x9fa5) {
      result.append(pinyin_list[data - 0x4e00]);
    } else {
      result.append(boost::locale::conv::utf_to_utf<char>(std::wstring{data}));
    }
  }
  // DOODLE_LOG_INFO(fmt::format("{} to {}", boost::locale::conv::utf_to_utf<char>(conStr), result));
  return result;
}

std::string convert::toEn(const wchar_t &conStr) {
  std::string result{};
  if (conStr >= 0x4e00 && conStr <= 0x9fa5) {
    result.append(pinyin_list[conStr - 0x4e00]);
  } else {
    result.append(boost::locale::conv::utf_to_utf<char>(std::wstring{conStr}));
  }
  return result;
}

convert &convert::Get() noexcept {
  static convert instance;
  return instance;
}

}  // namespace doodle
