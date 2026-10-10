#pragma once

// Simulator stand-in for the SDK's wolfSSL content-crypto backend. Hashes,
// AES-CBC and randomness are real (CommonCrypto); RSA and PKCS#12 report
// failure, so protected books stay a device-only test.
#include <CommonCrypto/CommonCryptor.h>
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonRandom.h>

#include <string>

#include "Crypto.h"

namespace freeink {
namespace content {

class WolfsslCrypto : public Crypto {
 public:
  std::string lastError;

  int32_t rsaPrivateRaw(const uint8_t *, size_t, const uint8_t *, size_t, uint8_t *, size_t) override {
    lastError = "RSA unavailable in the simulator";
    return -1;
  }
  bool aes128CbcDecrypt(const uint8_t key[16], const uint8_t iv[16], const uint8_t *in, size_t len,
                        uint8_t *out) override {
    return cbc(kCCDecrypt, key, 16, iv, in, len, out);
  }
  bool aes256CbcDecrypt(const uint8_t key[32], const uint8_t iv[16], const uint8_t *in, size_t len,
                        uint8_t *out) override {
    return cbc(kCCDecrypt, key, 32, iv, in, len, out);
  }
  void sha1(const uint8_t *data, size_t len, uint8_t out[20]) override { CC_SHA1(data, static_cast<CC_LONG>(len), out); }
  void sha256(const uint8_t *data, size_t len, uint8_t out[32]) override {
    CC_SHA256(data, static_cast<CC_LONG>(len), out);
  }
  bool rsaGenerate(RsaKeyPairDer *) override {
    lastError = "RSA unavailable in the simulator";
    return false;
  }
  bool rsaPublicEncrypt(const uint8_t *, size_t, const uint8_t *, size_t, uint8_t *, size_t, size_t *) override {
    lastError = "RSA unavailable in the simulator";
    return false;
  }
  bool rsaPrivateSignRaw(const uint8_t *, size_t, const uint8_t[20], uint8_t[128]) override {
    lastError = "RSA unavailable in the simulator";
    return false;
  }
  bool aes128CbcEncrypt(const uint8_t key[16], const uint8_t iv[16], const uint8_t *in, size_t len,
                        uint8_t *out) override {
    return cbc(kCCEncrypt, key, 16, iv, in, len, out);
  }
  bool pkcs12Extract(const uint8_t *, size_t, const std::string &, std::vector<uint8_t> *,
                     std::vector<uint8_t> *) override {
    lastError = "PKCS#12 unavailable in the simulator";
    return false;
  }
  void randomBytes(uint8_t *out, size_t len) override { CCRandomGenerateBytes(out, len); }

 private:
  static bool cbc(CCOperation op, const uint8_t *key, size_t keyLen, const uint8_t iv[16], const uint8_t *in,
                  size_t len, uint8_t *out) {
    size_t moved = 0;
    return CCCrypt(op, kCCAlgorithmAES, 0, key, keyLen, iv, in, len, out, len, &moved) == kCCSuccess && moved == len;
  }
};

}  // namespace content
}  // namespace freeink
