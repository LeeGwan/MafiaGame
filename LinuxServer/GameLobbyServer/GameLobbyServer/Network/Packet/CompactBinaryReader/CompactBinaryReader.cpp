#include "CompactBinaryReader.h"
#include <cstddef>
#include <cstdint>
// 기본 생성자 (오브젝트 초기화)
CompactBinaryReader::CompactBinaryReader(){}
// 내부 상태 초기화
void CompactBinaryReader::clear() {p_data=nullptr;data_size=0;data_offset=0;}
// 내부 포인터 초기화
 void CompactBinaryReader::init(const uint8_t* payload_ptr,size_t size)
 {
   p_data=payload_ptr;
   data_size=size;
 }
// 지정된 바이트만큼 읽을 수 있는지 확인
bool CompactBinaryReader::HasData(size_t bytes) const {
  return data_offset + bytes <= data_size;
}
// 단일 바이트 읽기
uint8_t CompactBinaryReader::ReadUInt8() {
  return HasData(1) ? (p_data[data_offset++]) : 0;
}
// 2바이트 unsigned 읽기 (리틀 엔디안)
uint16_t CompactBinaryReader::ReadUInt16() {
  if (!HasData(2))
    return 0;
  uint16_t result = static_cast<uint16_t>(p_data[data_offset] |
                                          (p_data[data_offset + 1] << 8));
  data_offset += 2;
  return result;
}
// 2바이트 signed 읽기 (리틀 엔디안)
int16_t CompactBinaryReader::ReadInt16() {
  if (!HasData(2))
    return 0;
  int16_t result = static_cast<int16_t>(p_data[data_offset] |
                                        (p_data[data_offset + 1] << 8));
  data_offset += 2;
  return result;
}
// 4바이트 unsigned 읽기 (리틀 엔디안)
uint32_t CompactBinaryReader::ReadUInt32() {
  if (!HasData(4))
    return 0;
  uint32_t result = static_cast<uint32_t>(
      p_data[data_offset] | (p_data[data_offset + 1] << 8) |
      (p_data[data_offset + 2] << 16) | (p_data[data_offset + 3] << 24));
  data_offset += 4;
  return result;
}
// 4바이트 signed 읽기
int32_t CompactBinaryReader::ReadInt32() {
  return static_cast<int32_t>(ReadUInt32());
}
// 압축된 float 읽기 (정밀도 적용)
float CompactBinaryReader::ReadCompactFloat(float precision) {
  return static_cast<float>(ReadInt32()) * precision;
}
// 문자열 읽기 (uint16 length + data)
// 길이 검사: 요청된 길이가 없거나 100초과이면 빈 문자열 반환
std::string CompactBinaryReader::ReadString() {
  if (!HasData(2))
    return "";
  uint16_t length = p_data[data_offset] | (p_data[data_offset + 1] << 8);
  data_offset += 2;
  if (!HasData(length))
    return "";
  if (length > 100)
    return "";

  std::string result;
  result.reserve(length);
  for (uint16_t i = 0; i < length; ++i) {
    result.push_back(static_cast<char>(p_data[data_offset + i]));

  }
  data_offset += length;

  return result;
}
// 문자열 벡터 읽기 (uint8 count + 각 문자열)
std::vector<std::string> CompactBinaryReader::ReadStringVector() {
  uint8_t count = ReadUInt8();
  std::vector<std::string> result;
  result.reserve(count);
  for (uint8_t i = 0; i < count; ++i) {
    result.push_back(ReadString());
  }
  return result;
}
// 비트 플래그 읽기
uint8_t CompactBinaryReader::ReadBitFlags() { return ReadUInt8(); }
// bool 읽기
bool CompactBinaryReader::ReadBool() { return ReadUInt8() != 0; }
