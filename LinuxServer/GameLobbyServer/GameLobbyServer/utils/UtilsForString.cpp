#include "UtilsForString.h"
#include <bitset>
#include <algorithm>
#include <iostream>
bool UtilsForString::IsValidHash(const std::string &Hash) {

  if (Hash.empty() || Hash.size() != 64)
    return false;
  return true;
}
bool UtilsForString::IsValidETC(const std::string& ETC)
{
    if (ETC.empty() || ETC.size() > 40)
        return false;

    return true;
}
bool UtilsForString::IsValidID(const std::string &id) {

  if (id.empty() || id.size() >64)
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
bool UtilsForString::IsValidPassword(const std::string &pw) {
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