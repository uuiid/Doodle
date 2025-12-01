//
// Created by TD on 2022/3/23.
//

#include "authorization.h"

#include "doodle_core/core/core_set.h"
#include "doodle_core/metadata/time_point_wrap.h"
#include <doodle_core/logger/logger.h>
#include <doodle_core/platform/win/register_file_type.h>

#include "cryptopp/files.h"
#include "cryptopp/hex.h"
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/gcm.h>

namespace doodle {
class authorization::impl {
  friend void to_json(nlohmann::json& j, const impl& p) {
    j["time"]  = p.l_time;
    j["uuid1"] = core_set::get_set().get_uuid();
    j["uuid2"] = core_set::get_set().get_uuid();
    j["uuid3"] = core_set::get_set().get_uuid();
  }
  friend void from_json(const nlohmann::json& j, impl& p) { p.l_time = j.at("time").get<chrono::system_zoned_time>(); }

 public:
  chrono::system_zoned_time l_time{};
};

bool authorization::is_build_near() {
  /// 优先检查构建时间
  chrono::sys_seconds l_build_time_;
  std::istringstream l_time{version::build_info::get().build_time};
  l_time >> chrono::parse("%Y %m %d %H %M %S", l_build_time_);
  chrono::sys_time_pos l_point{l_build_time_};
  l_point += chrono::months{3};
  if (chrono::system_clock::now() < l_point) {
    p_i->l_time = l_point;
    return true;
  }
  return false;
}

void authorization::load_authorization_data(const std::string& in_data) {
  if (in_data.empty()) return;
  std::string decryptedtext{};
  try {
    CryptoPP::GCM<CryptoPP::AES>::Decryption l_decryption{};
    l_decryption.SetKeyWithIV(
        doodle_config::cryptopp_key.data(), doodle_config::cryptopp_key.size(), doodle_config::cryptopp_iv.data(),
        doodle_config::cryptopp_iv.size()
    );

    CryptoPP::StringSource l_file{
        in_data, true,
        new CryptoPP::HexDecoder{new CryptoPP::AuthenticatedDecryptionFilter{
            l_decryption, new CryptoPP::StringSink{decryptedtext},
            CryptoPP::AuthenticatedDecryptionFilter::DEFAULT_FLAGS, doodle_config::cryptopp_tag_size
        }}
    };
    //    l_file.Flush(true);
  } catch (const CryptoPP::Exception& error) {
    log_error(fmt::format("解析授权码错误 : {}", error.what()));
    return;
  }
  try {
    *p_i = nlohmann::json::parse(decryptedtext).get<impl>();
  } catch (...) {
    default_logger_raw()->error("解析授权码错误 : {}", boost::current_exception_diagnostic_information());
  }
}

authorization::authorization(const std::string& in_data) : p_i(std::make_shared<impl>()) {
  if (is_build_near()) {
    default_logger_raw()->info("使用构建时间授权");
    return;
  }
  load_authorization_data(in_data);
}

bool authorization::is_valid() const { return p_i->l_time.get_sys_time() > chrono::system_clock::now(); }
void authorization::generate_token(const FSys::path& in_path) {
  DOODLE_CHICK(!in_path.empty(), doodle_error{"传入路径为空"});
  impl l_impl{};
  l_impl.l_time           = chrono::system_clock::now();
  nlohmann::json out_json = l_impl;

  {
    FSys::ofstream l_f_s{
        in_path / FSys::path{doodle_config::token_name.data()},
        std::ofstream::binary | std::ofstream::out | std::ofstream::trunc
    };

    CryptoPP::GCM<CryptoPP::AES>::Encryption aes_Encryption{};
    aes_Encryption.SetKeyWithIV(
        doodle_config::cryptopp_key.data(), doodle_config::cryptopp_key.size(), doodle_config::cryptopp_iv.data(),
        doodle_config::cryptopp_iv.size()
    );
    CryptoPP::StringSource l_string_source{
        out_json.dump(), true,
        new CryptoPP::AuthenticatedEncryptionFilter{
            aes_Encryption, new CryptoPP::HexEncoder{new CryptoPP::FileSink{l_f_s}}, false,
            doodle_config::cryptopp_tag_size
        }
    };
  }
}

chrono::sys_time_pos::duration authorization::get_expire_time() const {
  return p_i->l_time.get_sys_time() - chrono::system_clock::now();
}

}  // namespace doodle
