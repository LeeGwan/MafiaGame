
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
class Vec3;
class CompactBinaryReader {
private:
  const uint8_t *p_data;
  size_t data_size;
  size_t data_offset;

public:
  CompactBinaryReader();
  // 페이로드 초기화
  void init(const uint8_t* payload_ptr,size_t size);
  // 주어진 바이트를 읽을 수 있는지 확인
  bool HasData(size_t bytes) const;
  // 초기화 해제
  void clear();

  // 읽기 함수들
  uint8_t ReadUInt8();
  int16_t ReadInt16();
  uint16_t ReadUInt16();
  uint32_t ReadUInt32();
  int32_t ReadInt32();
  float ReadCompactFloat(float precision = 0.01f);
  std::string ReadString();
  std::vector<std::string> ReadStringVector();
  uint8_t ReadBitFlags();
  bool ReadBool();

  
};
