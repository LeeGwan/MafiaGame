/**
 * @file UtilsForString.h
 * @brief Header for string validation, sanitization, and encoding conversion utilities.
 */

#pragma once
#include <string>

/**
 * @class UtilsForString
 * @brief Static utility class providing robust string manipulation and validation routines.
 * * This class implements strict whitelisting for security-sensitive inputs and 
 * provides cross-platform encoding conversions (UTF-8/UTF-16).
 */
class UtilsForString {
public:

    /**
     * @brief Validates a session token/hash string.
     * @param hash The 64-character hash string to verify.
     * @return True if the hash conforms to the required length and format.
     */
    static bool IsValidHash(const std::string& hash);

    /**
     * @brief Validates general-purpose auxiliary strings.
     * @param ETC The input string for generic metadata or labels.
     * @return True if the string is non-empty and within the 40-character limit.
     */
    static bool IsValidETC(const std::string& ETC);

    /**
     * @brief Validates a User Identifier (ID).
     * * Enforces an alphanumeric-only policy and a 50-character maximum length.
     * @param id The ID string to sanitize.
     * @return True if the ID is compliant.
     */
    static bool IsValidID(const std::string& id);

    /**
     * @brief Validates a User Password string.
     * * Enforces complexity requirements (Alphanumeric + Special Characters) 
     * and a 50-character maximum length.
     * @param pw The password string to sanitize.
     * @return True if the password meets security requirements.
     */
    static bool IsValidPassword(const std::string& pw);

    /**
     * @brief Converts a Wide-String (UTF-16) to a Multi-byte (UTF-8) encoded string.
     * @param wstr The wide-string to convert.
     * @return The resulting UTF-8 encoded std::string.
     */
    static std::string WStringToUTF8(const std::wstring& wstr);

    /**
     * @brief Converts a Multi-byte (UTF-8/ANSI) string to a Wide-String (UTF-16).
     * @param str The source string.
     * @param type The code page identifier (e.g., CP_UTF8, CP_ACP).
     * @return The resulting Wide-String (std::wstring).
     */
    static std::wstring UTF8ToWString(const std::string& str, int type);
};
