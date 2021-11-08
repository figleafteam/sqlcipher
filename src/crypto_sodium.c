/*
 ** SQLCipher
 ** http://sqlcipher.net
 **
 ** Copyright (c) 2008 - 2013, ZETETIC LLC, Rokas Kupstys
 ** All rights reserved.
 **
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions are met:
 **     * Redistributions of source code must retain the above copyright
 **       notice, this list of conditions and the following disclaimer.
 **     * Redistributions in binary form must reproduce the above copyright
 **       notice, this list of conditions and the following disclaimer in the
 **       documentation and/or other materials provided with the distribution.
 **     * Neither the name of the ZETETIC LLC nor the
 **       names of its contributors may be used to endorse or promote products
 **       derived from this software without specific prior written permission.
 **
 ** THIS SOFTWARE IS PROVIDED BY ZETETIC LLC ''AS IS'' AND ANY
 ** EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 ** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 ** DISCLAIMED. IN NO EVENT SHALL ZETETIC LLC BE LIABLE FOR ANY
 ** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 ** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 ** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 ** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **
 */
/* BEGIN SQLCIPHER */
#ifdef SQLITE_HAS_CODEC
#ifdef SQLCIPHER_CRYPTO_SODIUM
#include "crypto.h"
#include "sqlcipher.h"
#include <sodium.h>

static const char* sqlcipher_sodium_get_provider_name(void *ctx) {
  return "libsodium";
}

static const char* sqlcipher_sodium_get_provider_version(void *ctx) {
  return SODIUM_VERSION_STRING;
}

static int sqlcipher_sodium_activate(void *ctx) {
  // sodium_init() returns 0 on success, -1 on failure, and 1 if the library had already been initialized.
  return sodium_init() == -1 ? SQLITE_ERROR : SQLITE_OK;
}

static int sqlcipher_sodium_deactivate(void *ctx) {
  return SQLITE_OK;
}

static int sqlcipher_sodium_random(void *ctx, void *buffer, int length) {
  randombytes_buf(buffer, length);
  return SQLITE_OK;
}

static int sqlcipher_sodium_add_random(void *ctx, void *buffer, int length) {
  randombytes_stir();
  return SQLITE_OK;
}

static int sqlcipher_sodium_hmacsha256(unsigned char *hmac_key, int key_sz, unsigned char *in, int in_sz, unsigned char *in2, int in2_sz, unsigned char *out) {
  crypto_auth_hmacsha256_state state = { 0 };

  if (crypto_auth_hmacsha256_init(&state, hmac_key, key_sz) != 0) return SQLITE_ERROR;
  if (in != NULL && in_sz > 0 && crypto_auth_hmacsha256_update(&state, in, in_sz) != 0) return SQLITE_ERROR;
  if (in2 != NULL && in2_sz > 0 && crypto_auth_hmacsha256_update(&state, in2, in2_sz) != 0) return SQLITE_ERROR;
  if (crypto_auth_hmacsha256_final(&state, out) != 0) return SQLITE_ERROR;

  return SQLITE_OK;
}

static int sqlcipher_sodium_hmacsha512(unsigned char *hmac_key, int key_sz, unsigned char *in, int in_sz, unsigned char *in2, int in2_sz, unsigned char *out) {
  crypto_auth_hmacsha512_state state = { 0 };

  if (crypto_auth_hmacsha512_init(&state, hmac_key, key_sz) != 0) return SQLITE_ERROR;
  if (in != NULL && in_sz > 0 && crypto_auth_hmacsha512_update(&state, in, in_sz) != 0) return SQLITE_ERROR;
  if (in2 != NULL && in2_sz > 0 && crypto_auth_hmacsha512_update(&state, in2, in2_sz) != 0) return SQLITE_ERROR;
  if (crypto_auth_hmacsha512_final(&state, out) != 0) return SQLITE_ERROR;

  return SQLITE_OK;
}

static int sqlcipher_sodium_hmac(void *ctx, int algorithm, unsigned char *hmac_key, int key_sz, unsigned char *in, int in_sz, unsigned char *in2, int in2_sz, unsigned char *out) {
  switch (algorithm) {
    case SQLCIPHER_HMAC_SHA256:
      return sqlcipher_sodium_hmacsha256(hmac_key, key_sz, in, in_sz, in2, in2_sz, out);
    case SQLCIPHER_HMAC_SHA512:
      return sqlcipher_sodium_hmacsha512(hmac_key, key_sz, in, in_sz, in2, in2_sz, out);
    default:
      return SQLITE_ERROR;
  }
}

static int sqlcipher_sodium_kdf(void *ctx, int algorithm, const unsigned char *pass, int pass_sz, unsigned char* salt, int salt_sz, int workfactor, int key_sz, unsigned char *key) {
  if (crypto_pwhash(key, key_sz, pass, pass_sz, salt, crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE, crypto_pwhash_ALG_ARGON2I13) != 0) return SQLITE_ERROR;
  return SQLITE_OK;
}

static int sqlcipher_sodium_cipher(void *ctx, int mode, unsigned char *key, int key_sz, unsigned char *iv, unsigned char *in, int in_sz, unsigned char *out) {
  if (crypto_stream_xor(out, in, in_sz, iv, key) != 0) return SQLITE_ERROR;
  return SQLITE_OK;
}

static const char* sqlcipher_sodium_get_cipher(void *ctx) {
  return crypto_stream_PRIMITIVE;
}

static int sqlcipher_sodium_get_key_sz(void *ctx) {
  return crypto_stream_KEYBYTES;
}

static int sqlcipher_sodium_get_iv_sz(void *ctx) {
  return crypto_stream_NONCEBYTES;
}

static int sqlcipher_sodium_get_block_sz(void *ctx) {
  // Concrete value is not important so use AES-256 block size
  return 16;
}

static int sqlcipher_sodium_get_hmac_sz(void *ctx, int algorithm) {
  switch (algorithm) {
    case SQLCIPHER_HMAC_SHA256:
      return crypto_auth_hmacsha256_BYTES;
    case SQLCIPHER_HMAC_SHA512:
      return crypto_auth_hmacsha512_BYTES;
    default:
      return 0;
  }
}

static int sqlcipher_sodium_ctx_init(void **ctx) {
  return SQLITE_OK;
}

static int sqlcipher_sodium_ctx_free(void **ctx) {
  return SQLITE_OK;
}

static int sqlcipher_sodium_fips_status(void *ctx) {
  return 0;
}

int sqlcipher_sodium_setup(sqlcipher_provider *p) {
  p->get_provider_name = sqlcipher_sodium_get_provider_name;
  p->get_provider_version = sqlcipher_sodium_get_provider_version;
  p->activate = sqlcipher_sodium_activate;
  p->deactivate = sqlcipher_sodium_deactivate;
  p->random = sqlcipher_sodium_random;
  p->add_random = sqlcipher_sodium_add_random;
  p->hmac = sqlcipher_sodium_hmac;
  p->kdf = sqlcipher_sodium_kdf;
  p->cipher = sqlcipher_sodium_cipher;
  p->get_cipher = sqlcipher_sodium_get_cipher;
  p->get_key_sz = sqlcipher_sodium_get_key_sz;
  p->get_iv_sz = sqlcipher_sodium_get_iv_sz;
  p->get_block_sz = sqlcipher_sodium_get_block_sz;
  p->get_hmac_sz = sqlcipher_sodium_get_hmac_sz;
  p->ctx_init = sqlcipher_sodium_ctx_init;
  p->ctx_free = sqlcipher_sodium_ctx_free;
  p->fips_status = sqlcipher_sodium_fips_status;
  return SQLITE_OK;
}

#endif
#endif
/* END SQLCIPHER */
