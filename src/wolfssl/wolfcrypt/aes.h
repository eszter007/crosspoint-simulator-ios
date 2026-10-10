#pragma once

// Simulator stand-in for wolfCrypt's AES. CBC and SHA-256 are real
// (CommonCrypto); GCM is a process-consistent keystream stand-in so a key
// wrapped here unwraps here, which is all the simulator needs.
#include <CommonCrypto/CommonCryptor.h>
#include <CommonCrypto/CommonDigest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

typedef uint8_t byte;
typedef uint32_t word32;

#define AES_ENCRYPTION 0
#define AES_DECRYPTION 1
#define AES_BLOCK_SIZE 16
#define INVALID_DEVID (-2)

struct Aes {
  byte key[32] = {};
  word32 keyLen = 0;
  byte iv[16] = {};
};

inline int wc_AesInit(Aes *aes, void *, int) {
  if (aes) *aes = Aes{};
  return aes ? 0 : -1;
}
inline void wc_AesFree(Aes *) {}

inline int wc_AesSetKey(Aes *aes, const byte *key, word32 len, const byte *iv, int) {
  if (!aes || !key || (len != 16 && len != 24 && len != 32)) return -1;
  memcpy(aes->key, key, len);
  aes->keyLen = len;
  if (iv) memcpy(aes->iv, iv, 16);
  return 0;
}

inline int wc_AesCbcEncrypt(Aes *aes, byte *out, const byte *in, word32 sz) {
  size_t moved = 0;
  const CCCryptorStatus st = CCCrypt(kCCEncrypt, kCCAlgorithmAES, 0, aes->key, aes->keyLen, aes->iv, in, sz,
                                     out, sz, &moved);
  return st == kCCSuccess && moved == sz ? 0 : -1;
}

inline int wc_AesGcmSetKey(Aes *aes, const byte *key, word32 len) { return wc_AesSetKey(aes, key, len, nullptr, 0); }

namespace wolf_sim {
inline void keystream(const Aes *aes, const byte *iv, word32 ivSz, word32 counter, byte out[32]) {
  byte seed[32 + 16 + 4];
  memcpy(seed, aes->key, 32);
  memset(seed + 32, 0, 16);
  memcpy(seed + 32, iv, ivSz < 16 ? ivSz : 16);
  memcpy(seed + 48, &counter, 4);
  CC_SHA256(seed, sizeof(seed), out);
}
inline void tag(const Aes *aes, const byte *data, word32 sz, const byte *aad, word32 aadSz, byte out[32]) {
  CC_SHA256_CTX ctx;
  CC_SHA256_Init(&ctx);
  CC_SHA256_Update(&ctx, aes->key, 32);
  CC_SHA256_Update(&ctx, aad, aadSz);
  CC_SHA256_Update(&ctx, data, sz);
  CC_SHA256_Final(out, &ctx);
}
inline void xorStream(const Aes *aes, byte *out, const byte *in, word32 sz, const byte *iv, word32 ivSz) {
  byte ks[32];
  for (word32 i = 0; i < sz; i++) {
    if (i % 32 == 0) keystream(aes, iv, ivSz, i / 32, ks);
    out[i] = in[i] ^ ks[i % 32];
  }
}
}  // namespace wolf_sim

inline int wc_AesGcmEncrypt(Aes *aes, byte *out, const byte *in, word32 sz, const byte *iv, word32 ivSz,
                            byte *authTag, word32 authTagSz, const byte *authIn, word32 authInSz) {
  wolf_sim::xorStream(aes, out, in, sz, iv, ivSz);
  byte t[32];
  wolf_sim::tag(aes, out, sz, authIn, authInSz, t);
  memcpy(authTag, t, authTagSz < 32 ? authTagSz : 32);
  return 0;
}

inline int wc_AesGcmDecrypt(Aes *aes, byte *out, const byte *in, word32 sz, const byte *iv, word32 ivSz,
                            const byte *authTag, word32 authTagSz, const byte *authIn, word32 authInSz) {
  byte t[32];
  wolf_sim::tag(aes, in, sz, authIn, authInSz, t);
  if (memcmp(t, authTag, authTagSz < 32 ? authTagSz : 32) != 0) return -1;
  wolf_sim::xorStream(aes, out, in, sz, iv, ivSz);
  return 0;
}
