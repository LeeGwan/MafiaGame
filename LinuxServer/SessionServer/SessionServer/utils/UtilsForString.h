#ifndef UTILSFORSTRING_H
#define UTILSFORSTRING_H

#pragma once
#include <string>
class UtilsForString {
public:
	static bool IsValidETC(const std::string& ETC);
  static bool IsValidHash(const std::string &hash);
  static bool IsValidID(const std::string &id);
  static bool IsValidPassword(const std::string &pw);
};

#endif