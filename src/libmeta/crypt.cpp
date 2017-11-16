// =================================================================================================
///  @file crypt.h
///
///  Handles encrypt/decrypt functions for ODBC
///
///  Copyright (C) 2017 Ryft Systems, Inc.
// =================================================================================================

#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <string.h>

const char keypass[] = "nu!Ag2n&RxFAjXj^";

char *__encrypt(const char *in)
{
    char *out;
    BIO *cipher, *b64, *buffer;
    char *mem;
    unsigned char key[16];
    unsigned char iv[SHA_DIGEST_LENGTH];

    OpenSSL_add_all_algorithms();

    cipher = BIO_new(BIO_f_cipher());
    b64 = BIO_new(BIO_f_base64());
    buffer = BIO_new(BIO_s_mem());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    EVP_BytesToKey(EVP_aes_128_gcm(), EVP_sha1(), NULL, (const unsigned char *)keypass, strlen(keypass), 1, key, iv);
    BIO_set_cipher(cipher, EVP_aes_128_gcm(), key, iv, 1);
    BIO_push(cipher, b64);
    BIO_push(b64, buffer);
    BIO_write(cipher, in, strlen(in));
    BIO_flush(cipher);
    size_t len = BIO_get_mem_data(buffer, &mem);
    out = (char *)malloc(len+1);
    strcpy(out, mem);
    BIO_free_all(cipher);
    return out;
}

char * __decrypt(const char *in)
{
    BIO *buffer, *b64, *cipher;
    char *mem;
    unsigned char key[16];
    unsigned char iv[SHA_DIGEST_LENGTH];
    char *out = (char *)malloc(strlen(in)+1);

    OpenSSL_add_all_algorithms();

    buffer = BIO_new_mem_buf((void *)in, -1);
    b64 = BIO_new(BIO_f_base64());
    cipher = BIO_new(BIO_f_cipher());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    EVP_BytesToKey(EVP_aes_128_gcm(), EVP_sha1(), NULL, (const unsigned char *)keypass, strlen(keypass), 1, key, iv);
    BIO_set_cipher(cipher, EVP_aes_128_gcm(), key, iv, 0);
    BIO_push(cipher, b64);
    BIO_push(b64, buffer);
    size_t length = BIO_read(cipher, out, strlen(in));
    out[length] = '\0';
    BIO_free_all(cipher);
    return out;
}
