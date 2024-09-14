#include <doodle_core/core/core_set.h>
#include <core/filesystem_extend.h>
#include <cryptopp/base64.h>
#include <cryptopp/files.h>
#include <cryptopp/hex.h>
#include <cryptopp/sha.h>

#include <boost/locale.hpp>
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

std::string file_hash_sha224(const path& in_file) {
  (FSys::exists(in_file) && FSys::is_regular_file(in_file))
      ? void()
      : throw_exception(doodle_error{"{} 文件不存在或者不是文件", in_file});

  CryptoPP::SHA224 k_sha_224;
  std::string k_string;
  ifstream k_ifstream{in_file, std::ios::binary | std::ios::in};
  DOODLE_CHICK(k_ifstream, doodle_error{"{} 无法打开", in_file});

  CryptoPP::FileSource k_file{
      k_ifstream,
      true,
      new CryptoPP::HashFilter{
          k_sha_224,
          new CryptoPP::HexEncoder{
              new CryptoPP::StringSink{k_string}}}};
  return k_string;
}
}  // namespace FSys
}  // namespace doodle
