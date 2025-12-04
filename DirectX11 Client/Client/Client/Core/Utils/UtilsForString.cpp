// 문자열 검증 및 변환 유틸리티 구현
#include "UtilsForString.h"
#include <bitset>
#include <algorithm>
#include <codecvt>
#include <locale>
#include <Windows.h>

// 세션 토큰 검증 (64자 해시)
bool UtilsForString::IsValidHash(const std::string& Hash) {
    if (Hash.empty() || Hash.size() != 64)
        return false;
    return true;
}

// 기타 문자열 검증 (최대 40자)
bool UtilsForString::IsValidETC(const std::string& ETC)
{
    if (ETC.empty() || ETC.size() > 40)
        return false;
    return true;
}

// ID 검증 (영문/숫자만, 최대 50자)
bool UtilsForString::IsValidID(const std::string& id) {
    if (id.empty() || id.size() > 50)
        return false;

    static std::bitset<256> allowed;
    static bool initialized = false;
    if (!initialized) {
        for (int c = '0'; c <= '9'; ++c) allowed[c] = true;
        for (int c = 'A'; c <= 'Z'; ++c) allowed[c] = true;
        for (int c = 'a'; c <= 'z'; ++c) allowed[c] = true;
        initialized = true;
    }

    return std::all_of(id.begin(), id.end(), [&](unsigned char c) {
        return allowed[c];
        });
}

// 비밀번호 검증 (영문/숫자/특수문자, 최대 50자)
bool UtilsForString::IsValidPassword(const std::string& pw) {
    if (pw.empty() || pw.size() > 50)
        return false;

    static std::bitset<256> allowed;
    static bool initialized = false;
    if (!initialized) {
        for (int c = '0'; c <= '9'; ++c) allowed[c] = true;
        for (int c = 'A'; c <= 'Z'; ++c) allowed[c] = true;
        for (int c = 'a'; c <= 'z'; ++c) allowed[c] = true;
        for (char c : std::string("!\"#$%&'()")) allowed[(unsigned char)c] = true;
        initialized = true;
    }

    return std::all_of(pw.begin(), pw.end(), [&](unsigned char c) {
        return allowed[c];
        });
}

// WString을 UTF-8로 변환
std::string UtilsForString::WStringToUTF8(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

// UTF-8을 WString으로 변환
std::wstring UtilsForString::UTF8ToWString(const std::string& str, int type)
{
    if (str.empty()) return std::wstring();
    int size = MultiByteToWideChar(type, 0, str.c_str(), -1, nullptr, 0);
    if (size == 0) return std::wstring();
    std::wstring wstr(size - 1, 0);
    MultiByteToWideChar(type, 0, str.c_str(), -1, &wstr[0], size);
    return wstr;
}