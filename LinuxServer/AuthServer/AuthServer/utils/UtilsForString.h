#ifndef UTILSFORSTRING_H
#define UTILSFORSTRING_H

#pragma once
#include <string>
class UtilsForString {
public:
  // static std::string WStringToUTF8(const std::wstring& wstr);
  // static std::wstring UTF8ToWString(const std::string& utf8str);
  static bool IsValidHash(const std::string &hash);
  static bool IsValidID(const std::string &id);
  static bool IsValidPassword(const std::string &pw);
};

#endif