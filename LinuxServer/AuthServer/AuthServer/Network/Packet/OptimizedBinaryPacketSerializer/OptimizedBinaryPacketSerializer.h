#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
enum class PacketType : uint8_t;
struct AuthPacket;
struct ResultPacket;
struct ResultAndHashPacket;
struct HashPacket;
struct ServerInfoPacket;
class CompactBinaryReader;
// 바이너리 직렬화 최적화 처리 클래스
class OptimizedBinaryPacketSerializer {
public:
    static const uint16_t MAGIC_NUMBER = 0x2340; // 패킷 유효성 검증 값
    static const uint8_t VERSION = 2;            // 패킷 버전

  // XXHash 포함 패킷 헤더 구조
  struct SecurePacketHeader {
    uint16_t magic;
    uint8_t version;
    uint8_t type;
    uint32_t data_size;
    uint32_t xxhash;
  };

  // 패킷 헤더에 XXHash 래핑
  static void WrapPacketWithXXHash(uint8_t type, std::vector<uint8_t> *data,
                                   size_t size);
  // 바이트 추가
  static void push(std::vector<uint8_t> *buffer, uint8_t value, size_t &offset);

  // 기본 데이터 타입 직렬화 함수들
  static void SerializeString(std::vector<uint8_t> *buffer,
                              const std::string &str, size_t &offset);
  static void SerializeUInt8(std::vector<uint8_t> *buffer, uint8_t value,
                             size_t &offset);
  static void SerializeInt16(std::vector<uint8_t> *buffer, int16_t value,
                             size_t &offset);
  static void SerializeUInt16(std::vector<uint8_t> *buffer, uint16_t value,
                              size_t &offset);
  static void SerializeInt32(std::vector<uint8_t> *buffer, int32_t value,
                             size_t &offset);
  static void SerializeUInt32(std::vector<uint8_t> *buffer, uint32_t value,
                              size_t &offset);
  static void SerializeFloat(std::vector<uint8_t> *buffer, float value,
                             size_t &offset);
  static void SerializeCompactFloat(std::vector<uint8_t> *buffer, float value,
                                    float precision = 0.01f);
  static void SerializeBool(std::vector<uint8_t> *buffer, bool value,
                            size_t &offset);
  static void SerializeBitFlags(std::vector<uint8_t> *buffer, uint8_t flags,
                                size_t &offset);
  static void SerializeStringVector(std::vector<uint8_t> *buffer,
                                    const std::vector<std::string> &vec,
                                    size_t &offset);

  // 패킷 타입 변경
  static void ChangePacket(std::vector<uint8_t> &data,
                           PacketType Change_type);
  // 패킷 파싱
  static bool ParseSecurePacket(const std::vector<uint8_t> &data,
                                PacketType &out_type,
                                CompactBinaryReader *out_reader);
  // 템플릿 구조체 -> 데이터 직렬화
  template <typename T>
  static void SerializePacket(const T &data, std::vector<uint8_t> *buffer);
  // 템플릿 구조체 <- 데이터 역직렬화
  template <typename T>
  static void DeserializePacket(CompactBinaryReader &reader, T &out);
};