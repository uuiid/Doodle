//
// Created by TD on 2022/1/7.
//

#include <doodle_lib/doodle_lib_all.h>

#include <catch.hpp>
#include <iostream>
#include <iomanip>

#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/filters.h>

using namespace doodle;
TEST_CASE("json") {
  nlohmann::json k_j{};
  k_j[0] = "sdsads";
  std::cout << k_j.dump() << std::endl;
}

TEST_CASE("Crypto_aes") {
  // Key and IV setup
  // AES encryption uses a secret key of a variable length (128-bit, 196-bit or 256-
  // bit). This key is secretly exchanged between two parties before communication
  // begins. DEFAULT_KEYLENGTH= 16 bytes
  CryptoPP::byte key[CryptoPP::AES::DEFAULT_KEYLENGTH], iv[CryptoPP::AES::BLOCKSIZE];
  memset(key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH);
  memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE);
  std::string a_data{"a_data"};
  const int TAG_SIZE    = 16;

  //
  // String and Sink setup
  //
  std::string plaintext = "Now is the time for all good men to come to the aide...";
  std::string ciphertext;
  std::string decryptedtext;

  //
  // Dump Plain Text
  //
  std::cout << "Plain Text (" << plaintext.size() << " bytes)" << std::endl;
  std::cout << plaintext;
  std::cout << std::endl
            << std::endl;

  //
  // Create Cipher Text
  //

  {
    CryptoPP::GCM<CryptoPP::AES>::Encryption aes_Encryption{};
    aes_Encryption.SetKeyWithIV(key, CryptoPP::AES::DEFAULT_KEYLENGTH, iv, CryptoPP::AES::BLOCKSIZE);
    CryptoPP::AuthenticatedEncryptionFilter l_authenticated_encryption_filter{
        aes_Encryption,
        new CryptoPP::StringSink{ciphertext},
        false,
        TAG_SIZE};

    l_authenticated_encryption_filter.ChannelPut(CryptoPP::AAD_CHANNEL, (const byte*)a_data.data(), a_data.size());
    l_authenticated_encryption_filter.ChannelMessageEnd(CryptoPP::AAD_CHANNEL);

    l_authenticated_encryption_filter.ChannelPut(CryptoPP::DEFAULT_CHANNEL, (const byte*)plaintext.data(), plaintext.size());
    l_authenticated_encryption_filter.ChannelMessageEnd(CryptoPP::DEFAULT_CHANNEL);
  }
  //  ciphertext.clear();

#ifdef DOODLE_
  {
    CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv);

    CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(ciphertext));
    stfEncryptor.Put(reinterpret_cast<const unsigned char*>(plaintext.c_str()), plaintext.length());
    stfEncryptor.MessageEnd();
  }
#endif

  //
  // Dump Cipher Text
  //
  std::cout << "Cipher Text (" << ciphertext.size() << " bytes)" << std::endl;

  for (char i : ciphertext) {
    std::cout << "0x" << std::hex << (0xFF & static_cast<CryptoPP::byte>(i)) << " ";
  }

  std::cout << std::endl
            << std::endl;
#ifdef DOODLE_
  //
  // Decrypt
  //
  CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);

  CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedtext));
  stfDecryptor.Put(reinterpret_cast<const unsigned char*>(ciphertext.c_str()), ciphertext.size());
  stfDecryptor.MessageEnd();
#endif
  {
    CryptoPP::GCM<CryptoPP::AES>::Decryption l_decryption{};
    l_decryption.SetKeyWithIV(key, CryptoPP::AES::DEFAULT_KEYLENGTH, iv, CryptoPP::AES::BLOCKSIZE);

    const string& enc = ciphertext.substr(0, ciphertext.length() - TAG_SIZE);
    const string& mac = ciphertext.substr(ciphertext.length() - TAG_SIZE);

    assert(ciphertext.size() == enc.size() + mac.size());
    assert(enc.size() == plaintext.size());
    assert(TAG_SIZE == mac.size());

    CryptoPP::AuthenticatedDecryptionFilter df{l_decryption, new CryptoPP::StringSink(decryptedtext), CryptoPP::AuthenticatedDecryptionFilter::MAC_AT_BEGIN | CryptoPP::AuthenticatedDecryptionFilter::THROW_EXCEPTION, TAG_SIZE};

    df.ChannelPut(CryptoPP::DEFAULT_CHANNEL, (const byte*)mac.data(), mac.size());
    df.ChannelPut(CryptoPP::AAD_CHANNEL, (const byte*)a_data.data(), a_data.size());
    df.ChannelPut(CryptoPP::DEFAULT_CHANNEL, (const byte*)enc.data(), enc.size());

    df.ChannelMessageEnd(CryptoPP::AAD_CHANNEL);
    df.ChannelMessageEnd(CryptoPP::DEFAULT_CHANNEL);
  }

  //
  // Dump Decrypted Text
  //
  std::cout << "Decrypted Text: " << std::endl;
  std::cout << decryptedtext;
  std::cout << std::endl
            << std::endl;
}
