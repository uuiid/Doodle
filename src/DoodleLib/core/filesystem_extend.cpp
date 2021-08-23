#include <core/CoreSet.h>
#include <core/filesystem_extend.h>

namespace doodle {
namespace FSys {
std::tuple<std::uint64_t, std::uint64_t> find_path_frame(const path& in_path) {
  static std::regex reg{R"(_(\d+)-(\d+)[\._])"};
  std::smatch k_match{};
  auto str   = in_path.generic_string();
  auto k_tup = std::make_tuple(1001, 1250);
  if (std::regex_search(str, k_match, reg)) {
    k_tup = std::make_tuple(std::stoi(k_match[1].str()), std::stoi(k_match[2].str()));
  }
  return k_tup;
}

FSys::path write_tmp_file(const std::string& in_falg,
                          const std::string& in_string,
                          const std::string& in_extension) {
  const static auto tmp_path = CoreSet::getSet().getCacheRoot(
      fmt::format("{}/v{}{}{}",
                  in_falg,
                  Doodle_VERSION_MAJOR,
                  Doodle_VERSION_MINOR,
                  Doodle_VERSION_PATCH));
  auto k_tmp_path = tmp_path / (boost::uuids::to_string(CoreSet::getSet().getUUID()) + in_extension);
  {  //写入文件后直接关闭
    FSys::fstream file{k_tmp_path, std::ios::out};
    file << in_string;
  }
  return k_tmp_path;
}
}  // namespace FSys
}  // namespace doodle