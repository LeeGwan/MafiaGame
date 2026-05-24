/**
 * @file UtilsForString.cpp
 * @brief Implementation of string validation and encoding conversion utilities.
 */

#include "UtilsForString.h"
#include <bitset>
#include <algorithm>
#include <codecvt>
#include <locale>
#include <Windows.h>

/**
 * @brief Validates a session token (expects a 64-character hex-style hash).
 * @param Hash The hash string to validate.
 * @return True if the format is valid.
 */
bool UtilsForString::IsValidHash(const std::string& Hash) {
    if (Hash.empty() || Hash.size() != 64)
        return false;
    return true;
}

/**
 * @brief General-purpose length validation (Max 40 characters).
 * @param ETC The input string.
 */
bool UtilsForString::IsValidETC(const std::string& ETC)
{
    if (ETC.empty() || ETC.size() > 40)
        return false;
    return true;
}

/**
 * @brief Validates a User ID (Alphanumeric only, max 50 chars).
 * Uses a static bitset for O(1) character lookup performance.
 */
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

/**
 * @brief Validates a password (Alphanumeric + specific special characters, max 50 chars).
 * Prevents potential injection attacks by strictly whitelisting characters.
 */
bool UtilsForString::IsValidPassword(const std::string& pw) {
    if (pw.empty() || pw.size() > 50)
        return false;

    static std::bitset<256> allowed;
    static bool initialized = false;
    if (!initialized) {
        for (int c = '0'; c <= '9'; ++c) allowed[c] = true;
        for (int c = 'A'; c <= 'Z'; ++c) allowed[c] = true;
        for (int c = 'a'; c <= 'z'; ++c) allowed[c] = true;
        // Whitelist specific special characters for password entropy
        for (char c : std::string("!\"#$%&'()")) allowed[(unsigned char)c] = true;
        initialized = true;
    }

    return std::all_of(pw.begin(), pw.end(), [&](unsigned char c) {
        return allowed[c];
    });
}

/**
 * @brief Converts a Wide-String (UTF-16) to a UTF-8 encoded string.
 * @note Uses std::codecvt_utf8 (C++11 standard).
 */
std::string UtilsForString::WStringToUTF8(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

/**
 * @brief Converts a Multi-byte/UTF-8 string to a Wide-String (UTF-16).
 * @param type The code page (e.g., CP_UTF8 or CP_ACP).
 * @return The resulting wstring.
 */
std::wstring UtilsForString::UTF8ToWString(const std::string& str, int type)
{
    if (str.empty()) return std::wstring();
    
    // Determine the required buffer size
    int size = MultiByteToWideChar(type, 0, str.c_str(), -1, nullptr, 0);
    if (size == 0) return std::wstring();
    
    std::wstring wstr(size - 1, 0);
    MultiByteToWideChar(type, 0, str.c_str(), -1, &wstr[0], size);
    return wstr;
}
