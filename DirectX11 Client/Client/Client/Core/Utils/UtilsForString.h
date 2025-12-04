// 문자열 검증 및 변환 유틸리티 클래스
#pragma once
#include <string>
class UtilsForString {
public:

	// 세션 토큰 검증 (64자 해시)
	static bool IsValidHash(const std::string& hash);
	// 기타 문자열 검증 (최대 40자)
	static bool IsValidETC(const std::string& ETC);
	// ID 검증 (영문/숫자, 최대 50자)
	static bool IsValidID(const std::string& id);
	// 비밀번호 검증 (영문/숫자/특수문자, 최대 50자)
	static bool IsValidPassword(const std::string& pw);
	// WString을 UTF-8로 변환
	static std::string WStringToUTF8(const std::wstring& wstr);
	// UTF-8을 WString으로 변환
	static std::wstring UTF8ToWString(const std::string& str, int type);
};