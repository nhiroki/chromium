// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <openssl/aes.h>
#include <openssl/evp.h>

#include "base/logging.h"
#include "base/macros.h"
#include "base/numerics/safe_math.h"
#include "base/stl_util.h"
#include "content/child/webcrypto/crypto_data.h"
#include "content/child/webcrypto/openssl/aes_key_openssl.h"
#include "content/child/webcrypto/openssl/key_openssl.h"
#include "content/child/webcrypto/openssl/util_openssl.h"
#include "content/child/webcrypto/status.h"
#include "content/child/webcrypto/webcrypto_util.h"
#include "crypto/openssl_util.h"
#include "crypto/scoped_openssl_types.h"
#include "third_party/WebKit/public/platform/WebCryptoAlgorithmParams.h"

namespace content {

namespace webcrypto {

namespace {

const EVP_CIPHER* GetAESCipherByKeyLength(unsigned int key_length_bytes) {
  // BoringSSL does not support 192-bit AES keys.
  switch (key_length_bytes) {
    case 16:
      return EVP_aes_128_ctr();
    case 32:
      return EVP_aes_256_ctr();
    default:
      return NULL;
  }
}

// Encrypts/decrypts given a 128-bit counter.
//
// |output| must be a pointer to a buffer which has a length of at least
// |input.byte_length()|.
Status AesCtrEncrypt128BitCounter(const EVP_CIPHER* cipher,
                                  const CryptoData& raw_key,
                                  const CryptoData& input,
                                  const CryptoData& counter,
                                  uint8_t* output) {
  DCHECK(cipher);
  DCHECK_EQ(16u, counter.byte_length());

  crypto::OpenSSLErrStackTracer err_tracer(FROM_HERE);
  crypto::ScopedOpenSSL<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free>::Type context(
      EVP_CIPHER_CTX_new());

  if (!context.get())
    return Status::OperationError();

  if (!EVP_CipherInit_ex(context.get(),
                         cipher,
                         NULL,
                         raw_key.bytes(),
                         counter.bytes(),
                         ENCRYPT)) {
    return Status::OperationError();
  }

  int output_len = 0;
  if (!EVP_CipherUpdate(context.get(),
                        output,
                        &output_len,
                        input.bytes(),
                        input.byte_length())) {
    return Status::OperationError();
  }
  int final_output_chunk_len = 0;
  if (!EVP_CipherFinal_ex(
          context.get(), output + output_len, &final_output_chunk_len)) {
    return Status::OperationError();
  }

  output_len += final_output_chunk_len;
  if (static_cast<unsigned int>(output_len) != input.byte_length())
    return Status::ErrorUnexpected();

  return Status::Success();
}

// Returns ceil(a/b), where a and b are integers.
template <typename T>
T CeilDiv(T a, T b) {
  return a == 0 ? 0 : 1 + (a - 1) / b;
}

// Extracts the counter as a BIGNUM. The counter is the rightmost
// "counter_length_bits" of the block, interpreted as a big-endian number.
crypto::ScopedBIGNUM GetCounter(const CryptoData& counter_block,
                                unsigned int counter_length_bits) {
  unsigned int counter_length_remainder_bits = (counter_length_bits % 8);

  // If the counter is a multiple of 8 bits then can call BN_bin2bn() directly.
  if (counter_length_remainder_bits == 0) {
    unsigned int byte_length = counter_length_bits / 8;
    return crypto::ScopedBIGNUM(BN_bin2bn(
        counter_block.bytes() + counter_block.byte_length() - byte_length,
        byte_length,
        NULL));
  }

  // Otherwise make a copy of the counter and zero out the topmost bits so
  // BN_bin2bn() can be called with a byte stream.
  unsigned int byte_length = CeilDiv(counter_length_bits, 8u);
  std::vector<uint8_t> counter(
      counter_block.bytes() + counter_block.byte_length() - byte_length,
      counter_block.bytes() + counter_block.byte_length());
  counter[0] &= ~(0xFF << counter_length_remainder_bits);

  return crypto::ScopedBIGNUM(
      BN_bin2bn(&counter.front(), counter.size(), NULL));
}

// Returns a counter block with the counter bits all set all zero.
std::vector<uint8_t> BlockWithZeroedCounter(const CryptoData& counter_block,
                                            unsigned int counter_length_bits) {
  unsigned int counter_length_bytes = counter_length_bits / 8;
  unsigned int counter_length_bits_remainder = counter_length_bits % 8;

  std::vector<uint8_t> new_counter_block(
      counter_block.bytes(),
      counter_block.bytes() + counter_block.byte_length());

  unsigned int index = new_counter_block.size() - counter_length_bytes;
  memset(&new_counter_block.front() + index, 0, counter_length_bytes);

  if (counter_length_bits_remainder) {
    new_counter_block[index - 1] &= 0xFF << counter_length_bits_remainder;
  }

  return new_counter_block;
}

// This function does encryption/decryption for AES-CTR (encryption and
// decryption are the same).
//
// BoringSSL's interface for AES-CTR differs from that of WebCrypto. In
// WebCrypto the caller specifies a 16-byte counter block and designates how
// many of the right-most X bits to use as a big-endian counter. Whereas in
// BoringSSL the entire counter block is interpreted as a 128-bit counter.
//
// In AES-CTR, the counter block MUST be unique across all messages that are
// encrypted/decrypted. WebCrypto expects that the counter can start at any
// value, and is therefore permitted to wrap around to zero on overflow.
//
// Some care is taken to fail if the counter wraps back to an earlier value.
// However this protection is only enforced during a *single* call to
// encrypt/decrypt.
Status AesCtrEncryptDecrypt(const blink::WebCryptoAlgorithm& algorithm,
                            const blink::WebCryptoKey& key,
                            const CryptoData& data,
                            std::vector<uint8_t>* buffer) {
  const blink::WebCryptoAesCtrParams* params = algorithm.aesCtrParams();
  const std::vector<uint8_t>& raw_key =
      SymKeyOpenSsl::Cast(key)->raw_key_data();

  if (params->counter().size() != 16)
    return Status::ErrorIncorrectSizeAesCtrCounter();

  unsigned int counter_length_bits = params->lengthBits();
  if (counter_length_bits < 1 || counter_length_bits > 128)
    return Status::ErrorInvalidAesCtrCounterLength();

  // The output of AES-CTR is the same size as the input. However BoringSSL
  // expects buffer sizes as an "int".
  base::CheckedNumeric<int> output_max_len = data.byte_length();
  if (!output_max_len.IsValid())
    return Status::ErrorDataTooLarge();

  const EVP_CIPHER* const cipher = GetAESCipherByKeyLength(raw_key.size());
  if (!cipher)
    return Status::ErrorUnexpected();

  const CryptoData counter_block(params->counter());
  buffer->resize(output_max_len.ValueOrDie());

  // The total number of possible counter values is pow(2, counter_length_bits)
  crypto::ScopedBIGNUM num_counter_values(BN_new());
  if (!BN_lshift(num_counter_values.get(), BN_value_one(), counter_length_bits))
    return Status::ErrorUnexpected();

  crypto::ScopedBIGNUM current_counter =
      GetCounter(counter_block, counter_length_bits);

  // The number of AES blocks needed for encryption/decryption. The counter is
  // incremented this many times.
  crypto::ScopedBIGNUM num_output_blocks(BN_new());
  if (!BN_set_word(
          num_output_blocks.get(),
          CeilDiv(buffer->size(), static_cast<size_t>(AES_BLOCK_SIZE)))) {
    return Status::ErrorUnexpected();
  }

  // If the counter is going to be incremented more times than there are counter
  // values, fail. (Repeating values of the counter block is bad).
  if (BN_cmp(num_output_blocks.get(), num_counter_values.get()) > 0)
    return Status::ErrorAesCtrInputTooLongCounterRepeated();

  // This is the number of blocks that can be successfully encrypted without
  // overflowing the counter. Encrypting the subsequent block will need to
  // reset the counter to zero.
  crypto::ScopedBIGNUM num_blocks_until_reset(BN_new());

  if (!BN_sub(num_blocks_until_reset.get(),
              num_counter_values.get(),
              current_counter.get())) {
    return Status::ErrorUnexpected();
  }

  // If the counter can be incremented for the entire input without
  // wrapping-around, do it as a single call into BoringSSL.
  if (BN_cmp(num_blocks_until_reset.get(), num_output_blocks.get()) >= 0) {
    return AesCtrEncrypt128BitCounter(cipher,
                                      CryptoData(raw_key),
                                      data,
                                      counter_block,
                                      vector_as_array(buffer));
  }

  // Otherwise the encryption needs to be done in 2 parts. The first part using
  // the current counter_block, and the next part resetting the counter portion
  // of the block to zero.

  // This is guaranteed to fit in an "unsigned int" because input size in bytes
  // fits in an "unsigned int".
  BN_ULONG num_blocks_part1 = BN_get_word(num_blocks_until_reset.get());
  BN_ULONG input_size_part1 = num_blocks_part1 * AES_BLOCK_SIZE;
  DCHECK_LT(input_size_part1, data.byte_length());

  // Encrypt the first part (before wrap-around).
  Status status =
      AesCtrEncrypt128BitCounter(cipher,
                                 CryptoData(raw_key),
                                 CryptoData(data.bytes(), input_size_part1),
                                 counter_block,
                                 vector_as_array(buffer));
  if (status.IsError())
    return status;

  // Encrypt the second part (after wrap-around).
  std::vector<uint8_t> counter_block_part2 =
      BlockWithZeroedCounter(counter_block, counter_length_bits);

  return AesCtrEncrypt128BitCounter(
      cipher,
      CryptoData(raw_key),
      CryptoData(data.bytes() + input_size_part1,
                 data.byte_length() - input_size_part1),
      CryptoData(counter_block_part2),
      vector_as_array(buffer) + input_size_part1);
}

class AesCtrImplementation : public AesAlgorithm {
 public:
  AesCtrImplementation() : AesAlgorithm("CTR") {}

  virtual Status Encrypt(const blink::WebCryptoAlgorithm& algorithm,
                         const blink::WebCryptoKey& key,
                         const CryptoData& data,
                         std::vector<uint8_t>* buffer) const OVERRIDE {
    return AesCtrEncryptDecrypt(algorithm, key, data, buffer);
  }

  virtual Status Decrypt(const blink::WebCryptoAlgorithm& algorithm,
                         const blink::WebCryptoKey& key,
                         const CryptoData& data,
                         std::vector<uint8_t>* buffer) const OVERRIDE {
    return AesCtrEncryptDecrypt(algorithm, key, data, buffer);
  }
};

}  // namespace

AlgorithmImplementation* CreatePlatformAesCtrImplementation() {
  return new AesCtrImplementation;
}

}  // namespace webcrypto

}  // namespace content
