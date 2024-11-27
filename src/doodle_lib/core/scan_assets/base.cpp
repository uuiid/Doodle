//
// Created by TD on 2023/12/20.
//
#include <doodle_core/metadata/assets_file.h>

#include <core/scan_assets/base.h>
#include <cryptopp/base64.h>
#include <cryptopp/files.h>
#include <cryptopp/hex.h>
#include <cryptopp/sha.h>
namespace doodle::details {

std::string scan_category_t::file_hash(const std::string& in_data) {
  CryptoPP::SHA224 k_sha_224;
  std::string l_str{};
  CryptoPP::StringSource k_file{
      in_data, true, new CryptoPP::HashFilter{k_sha_224, new CryptoPP::HexEncoder{new CryptoPP::StringSink{l_str}}}
  };
  return l_str;
}

void scan_category_t::scan_file_hash(const scan_category_data_ptr& in_data) {}

}  // namespace doodle::details