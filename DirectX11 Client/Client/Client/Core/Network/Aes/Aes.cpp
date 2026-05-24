/**
 * @file Aes.cpp
 * @brief Implementation of AES-256-CBC encryption and decryption.
 * @details This module utilizes the OpenSSL EVP API to secure network packets.
 * * @note [Development Purpose]
 * This class was developed manually to explore the mechanics of symmetric encryption, 
 * Initialization Vectors (IV), and the cryptographic pipeline (Serialization -> Encryption).
 * In production-grade software, the use of TLS/SSL is standard to provide robust 
 * session management and key exchange.
 */

#include "Aes.h"
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>

/** Global Singleton Instance for Cryptographic Operations */
std::unique_ptr<Aes> AES = std::make_unique<Aes>();

/**
 * @brief Constructor: Static key and IV initialization.
 * * @warning 
 * In real-world applications, hardcoded keys and manual socket encryption 
 * are discouraged in favor of TLS/SSL handshakes. This implementation 
 * is intended for pedagogical purposes to understand data confidentiality.
 */
Aes::Aes()
    : Aes_key({0x53, 0x4D, 0x08, 0xC3, 0x64, 0xA6, 0xA4, 0x9F, 0x23, 0x7E, 0xCD,
               0x17, 0x3E, 0xDB, 0x28, 0x37, 0xB9, 0xBF, 0x92, 0xF5, 0x9F, 0x56,
               0x7C, 0xF1, 0xE5, 0xEF, 0xE9, 0xC3, 0x01, 0xBD, 0x4E, 0x7A}),
      Aes_iv({0x7C, 0x04, 0x88, 0x7F, 0xE4, 0xBE, 0xCC, 0xA9, 0x9D, 0x62, 0x6C,
              0x6D, 0x48, 0x3B, 0xBC, 0x71}) {}

Aes::~Aes() { Release(); }

/**
 * @brief Decrypts ciphertext back into original plaintext.
 */
void Aes::Aes_Decrypt(const std::vector<uint8_t> *ciphertext, std::vector<uint8_t> *plaintext) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len = 0;
    int plaintext_len = 0;

    plaintext->resize(ciphertext->size() + EVP_MAX_BLOCK_LENGTH);
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, Aes_key.data(), Aes_iv.data());

    EVP_DecryptUpdate(ctx, plaintext->data(), &len, ciphertext->data(), (int)ciphertext->size());
    plaintext_len = len;

    EVP_DecryptFinal_ex(ctx, plaintext->data() + len, &len);
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    plaintext->resize(plaintext_len);
}

/**
 * @brief Encrypts plaintext using AES-256-CBC.
 */
void Aes::Aes_Encrypt(const std::vector<uint8_t> *plaintext, std::vector<uint8_t> *ciphertext) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len = 0;
    int ciphertext_len = 0;

    ciphertext->resize(plaintext->size() + EVP_MAX_BLOCK_LENGTH);
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, Aes_key.data(), Aes_iv.data());

    EVP_EncryptUpdate(ctx, ciphertext->data(), &len, plaintext->data(), (int)plaintext->size());
    ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, ciphertext->data() + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    ciphertext->resize(ciphertext_len);
}

/**
 * @brief Securely wipes sensitive key and IV data from memory.
 */
void Aes::Release() {
    if (!Aes_key.empty()) {
        OPENSSL_cleanse(Aes_key.data(), Aes_key.size());
    }

    if (!Aes_iv.empty()) {
        OPENSSL_cleanse(OPENSSL_cleanse(Aes_iv.data(), Aes_iv.size()));
    }
}
