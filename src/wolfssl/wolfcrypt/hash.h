#pragma once

#include <CommonCrypto/CommonDigest.h>

#include <cstdint>

typedef uint8_t byte;
typedef uint32_t word32;

#define WC_SHA256_DIGEST_SIZE 32

inline int wc_Sha256Hash(const byte *data, word32 len, byte *hash) {
  CC_SHA256(data, len, hash);
  return 0;
}
