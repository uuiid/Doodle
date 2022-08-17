//
// Created by TD on 2022/3/23.
//

#include "authorization.h"

#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/core/core_set.h>
#include <boost/contract.hpp>

#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/filters.h>

namespace doodle {
class authorization::impl {
  friend void to_json(nlohmann::json& j, const impl& p) {
    j["time"] = p.l_time;
  }
  friend void from_json(const nlohmann::json& j, impl& p) {
    p.l_time = j.at("time").get<time_point_wrap>();
  }

 public:
  time_point_wrap l_time{};
  std::string ciphertext_data{};
};

authorization::authorization(std::string in_data)
    : p_i(std::make_unique<impl>()) {
  std::string ciphertext{std::move(in_data)};
  std::string decryptedtext{};

  {
    CryptoPP::GCM<CryptoPP::AES>::Decryption l_decryption{};
    l_decryption.SetKeyWithIV(doodle_config::cryptopp_key.data(),
                              CryptoPP::AES::DEFAULT_KEYLENGTH,
                              doodle_config::cryptopp_iv.data(),
                              CryptoPP::AES::BLOCKSIZE);

    const std::string& enc = ciphertext.substr(0, ciphertext.length() - doodle_config::cryptopp_tag_size);
    const std::string& mac = ciphertext.substr(ciphertext.length() - doodle_config::cryptopp_tag_size);

    DOODLE_CHICK(ciphertext.size() == enc.size() + mac.size(), doodle_error{"授权码解码失误"});
    DOODLE_CHICK(doodle_config::cryptopp_tag_size == mac.size(), doodle_error{"授权码解码失误"});

    CryptoPP::AuthenticatedDecryptionFilter df{
        l_decryption, new CryptoPP::StringSink(decryptedtext),
        CryptoPP::AuthenticatedDecryptionFilter::MAC_AT_BEGIN |
            CryptoPP::AuthenticatedDecryptionFilter::THROW_EXCEPTION,
        doodle_config::cryptopp_tag_size};

    df.ChannelPut(CryptoPP::DEFAULT_CHANNEL,
                  (const CryptoPP::byte*)mac.data(), mac.size());
    df.ChannelPut(CryptoPP::AAD_CHANNEL,
                  (const CryptoPP::byte*)doodle_config::authorization_data.data(),
                  doodle_config::authorization_data.size());
    df.ChannelPut(CryptoPP::DEFAULT_CHANNEL, (const CryptoPP::byte*)enc.data(), enc.size());

    df.ChannelMessageEnd(CryptoPP::AAD_CHANNEL);
    df.ChannelMessageEnd(CryptoPP::DEFAULT_CHANNEL);
  }
  *p_i                 = nlohmann::json::parse(decryptedtext).get<impl>();
  p_i->ciphertext_data = std::move(ciphertext);
}
authorization::~authorization() {
  save();
}
bool authorization::is_expire() const {
  return p_i->l_time > time_point_wrap::now();
}
void authorization::generate_token(const FSys::path& in_path) {
  boost::contract::check l_c =
      boost::contract::function()
          .precondition([&]() {
            DOODLE_CHICK(!in_path.empty(), doodle_error{"传入路径为空"});
          });

  impl l_impl{};
  l_impl.l_time           = time_point_wrap(chrono::system_clock::now() + chrono::months{3});
  nlohmann::json out_json = l_impl;
  /**
   * @brief 加密后输出的数据
   */
  std::string out_data{};
  /**
   * @brief 需要加密的json数据
   */
  std::string in_data{out_json.dump()};
  {
    CryptoPP::GCM<CryptoPP::AES>::Encryption aes_Encryption{};
    aes_Encryption.SetKeyWithIV(doodle_config::cryptopp_key.data(),
                                CryptoPP::AES::DEFAULT_KEYLENGTH,
                                doodle_config::cryptopp_iv.data(),
                                CryptoPP::AES::BLOCKSIZE);
    CryptoPP::AuthenticatedEncryptionFilter l_authenticated_encryption_filter{
        aes_Encryption,
        new CryptoPP::StringSink{out_data},
        false,
        doodle_config::cryptopp_tag_size};

    l_authenticated_encryption_filter
        .ChannelPut(CryptoPP::AAD_CHANNEL,
                    (const CryptoPP::byte*)doodle_config::authorization_data.data(),
                    doodle_config::authorization_data.size());
    l_authenticated_encryption_filter
        .ChannelMessageEnd(CryptoPP::AAD_CHANNEL);

    l_authenticated_encryption_filter
        .ChannelPut(CryptoPP::DEFAULT_CHANNEL,
                    (const CryptoPP::byte*)in_data.data(), in_data.size());
    l_authenticated_encryption_filter.ChannelMessageEnd(CryptoPP::DEFAULT_CHANNEL);
  }
  if (exists(in_path))
    create_directories(in_path);

  FSys::ofstream{in_path / FSys::path{doodle_config::token_name.data()}} << out_data;
}
void authorization::save(const FSys::path& in_path) const {
  if (!exists(in_path.parent_path()))
    create_directories(in_path.parent_path());
  FSys::ofstream{in_path} << p_i->ciphertext_data;
}
void authorization::save() const {
  save(core_set::getSet().get_doc() / FSys::path{doodle_config::token_name.data()});
}

}  // namespace doodle
