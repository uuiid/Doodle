#include <core/core_set.h>
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
  auto tmp_path = core_set::getSet().get_cache_root(
      fmt::format("{}/v{}{}{}",
                  in_falg,
                  version::version_major,
                  version::version_minor,
                  version::version_patch));
  auto k_tmp_path = tmp_path / (boost::uuids::to_string(core_set::getSet().get_uuid()) + in_extension);
  {  //写入文件后直接关闭
    FSys::fstream file{k_tmp_path, std::ios::out};
    file << in_string;
  }
  return k_tmp_path;
}
std::vector<path> FSys::list_files(const path& in_dir, const path& in_ext_name) {
  std::vector<path> l_r{};
  return std::vector<path>();
}
}  // namespace FSys
}  // namespace doodle
