#pragma once


#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
namespace FSys {
std::tuple<std::uint64_t, std::uint64_t> find_path_frame(const path& in_path);
[[nodiscard]] FSys::path write_tmp_file(
    const std::string& in_falg,
    const std::string& in_string,
    const std::string& in_extension);

DOODLELIB_API std::vector<path> list_files(const path &in_dir,const path& in_ext_name);

}  // namespace FSys

}  // namespace doodle
