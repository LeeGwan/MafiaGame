#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
// AES 암호화/복호화 클래스
class Aes {
public:
   Aes();
  ~Aes();
  // AES 복호화 수행
  void Aes_Decrypt(const std::vector<uint8_t> *ciphertext,std::vector<uint8_t> *plaintext);
  // AES 암호화 수행
  void Aes_Encrypt(const std::vector<uint8_t>* plaintext,std::vector<uint8_t>*ciphertext);
  // 메모리 정리
  void Release();
private:
	// 암호화 키
	std::vector<unsigned char> Aes_key;
	// 초기화 벡터(IV)
	std::vector<unsigned char> Aes_iv;
};
// 전역 AES 객체
extern std::unique_ptr<Aes> AES;