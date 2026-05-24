/**
 * @file Aes.h
 * @brief Header for the AES-256-CBC encryption/decryption utility class.
 */

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

/**
 * @class Aes
 * @brief Provides an interface for symmetric encryption using the OpenSSL EVP API.
 * * @note [Pedagogical Purpose]
 * This implementation was created to explore the internal mechanics of data 
 * confidentiality and cryptographic pipelines. In production-grade systems, 
 * standard TLS (Transport Layer Security) should be used for secure socket 
 * communication and automated key management.
 * * @warning 
 * In real-world applications, hardcoded keys and manual socket encryption 
 * are discouraged in favor of TLS/SSL handshakes. This implementation 
 * is intended for pedagogical purposes to understand data confidentiality.
 */
class Aes {
public:
    /** @brief Initializes internal state and cryptographic parameters. */
    Aes();
    
    /** @brief Securely cleanses sensitive keys and releases resources. */
    ~Aes();

    /**
     * @brief Decrypts the provided ciphertext into plaintext.
     * @param ciphertext Pointer to the encrypted byte vector.
     * @param plaintext Pointer to the vector where decrypted data will be stored.
     */
    void Aes_Decrypt(const std::vector<uint8_t>* ciphertext, std::vector<uint8_t>* plaintext);

    /**
     * @brief Encrypts the provided plaintext into ciphertext.
     * @param plaintext Pointer to the raw byte vector.
     * @param ciphertext Pointer to the vector where encrypted data will be stored.
     */
    void Aes_Encrypt(const std::vector<uint8_t>* plaintext, std::vector<uint8_t>* ciphertext);

    /** @brief Physically wipes the key and IV from memory using OPENSSL_cleanse. */
    void Release();

private:
    /** @brief The 256-bit symmetric key used for encryption/decryption. */
    std::vector<unsigned char> Aes_key;

    /** @brief The 128-bit Initialization Vector (IV) for CBC mode. */
    std::vector<unsigned char> Aes_iv;
};

/** @brief Global singleton accessor for the AES module. */
extern std::unique_ptr<Aes> AES;
