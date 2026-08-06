/* Copyright (c) 2019,2024,2025,2026 MariaDB Corporation
   Copyright (c) 2026 lefred (Frédéric Descamps)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License. */

#include "semver.h"
#include <cctype>
#include <cstdlib>
#include <limits>
#include <sstream>

namespace semver {

static bool number(const std::string &s, uint64_t *value)
{
  if (s.empty() || (s.size() > 1 && s[0] == '0')) return false;
  uint64_t n= 0;
  for (char c : s)
  {
    if (c < '0' || c > '9') return false;
    unsigned digit= static_cast<unsigned>(c - '0');
    if (n > (std::numeric_limits<uint64_t>::max() - digit) / 10) return false;
    n= n * 10 + digit;
  }
  *value= n;
  return true;
}

static bool identifiers(const std::string &s, bool pre,
                        std::vector<std::string> *parts= nullptr)
{
  if (s.empty()) return false;
  size_t start= 0;
  while (start <= s.size())
  {
    size_t end= s.find('.', start);
    if (end == std::string::npos) end= s.size();
    std::string part= s.substr(start, end - start);
    if (part.empty()) return false;
    bool all_digits= true;
    for (char c : part)
    {
      if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-') return false;
      if (!std::isdigit(static_cast<unsigned char>(c))) all_digits= false;
    }
    if (pre && all_digits && part.size() > 1 && part[0] == '0') return false;
    if (parts) parts->push_back(part);
    if (end == s.size()) break;
    start= end + 1;
  }
  return true;
}

bool parse(const char *input, size_t length, Version *out, bool loose)
{
  if (!input || !out || !length || length > MAX_VERSION_LENGTH) return false;
  std::string s(input, length);
  if (loose)
  {
    size_t first= s.find_first_not_of(" \t\r\n");
    size_t last= s.find_last_not_of(" \t\r\n");
    if (first == std::string::npos) return false;
    s= s.substr(first, last - first + 1);
    if (!s.empty() && (s[0] == 'v' || s[0] == 'V' || s[0] == '=')) s.erase(0, 1);
  }
  if (s.empty() || s.find_first_of(" \t\r\n") != std::string::npos) return false;

  std::string core= s, pre, build;
  size_t plus= core.find('+');
  if (plus != std::string::npos)
  {
    build= core.substr(plus + 1); core.resize(plus);
    if (build.find('+') != std::string::npos || !identifiers(build, false)) return false;
  }
  size_t dash= core.find('-');
  if (dash != std::string::npos)
  {
    pre= core.substr(dash + 1); core.resize(dash);
    if (!identifiers(pre, true, &out->prerelease)) return false;
  }
  else out->prerelease.clear();

  std::vector<std::string> nums;
  size_t start= 0;
  while (start <= core.size())
  {
    size_t end= core.find('.', start);
    if (end == std::string::npos) end= core.size();
    nums.push_back(core.substr(start, end - start));
    if (end == core.size()) break;
    start= end + 1;
  }
  if ((!loose && nums.size() != 3) || (loose && (nums.empty() || nums.size() > 3))) return false;
  while (loose && nums.size() < 3) nums.push_back("0");
  if (!number(nums[0], &out->major) || !number(nums[1], &out->minor) ||
      !number(nums[2], &out->patch)) return false;
  out->build= build;
  std::ostringstream normalized;
  normalized << out->major << '.' << out->minor << '.' << out->patch;
  if (!pre.empty()) normalized << '-' << pre;
  if (!build.empty()) normalized << '+' << build;
  out->text= normalized.str();
  return out->text.size() <= MAX_VERSION_LENGTH;
}

static bool numeric_id(const std::string &s)
{
  for (char c : s) if (c < '0' || c > '9') return false;
  return true;
}

int compare(const Version &a, const Version &b)
{
  if (a.major != b.major) return a.major < b.major ? -1 : 1;
  if (a.minor != b.minor) return a.minor < b.minor ? -1 : 1;
  if (a.patch != b.patch) return a.patch < b.patch ? -1 : 1;
  if (a.prerelease.empty() != b.prerelease.empty()) return a.prerelease.empty() ? 1 : -1;
  for (size_t i= 0; i < a.prerelease.size() && i < b.prerelease.size(); ++i)
  {
    const std::string &x= a.prerelease[i], &y= b.prerelease[i];
    bool xn= numeric_id(x), yn= numeric_id(y);
    if (xn && yn)
    {
      if (x.size() != y.size()) return x.size() < y.size() ? -1 : 1;
      if (x != y) return x < y ? -1 : 1;
    }
    else if (xn != yn) return xn ? -1 : 1;
    else if (x != y) return x < y ? -1 : 1;
  }
  if (a.prerelease.size() == b.prerelease.size()) return 0;
  return a.prerelease.size() < b.prerelease.size() ? -1 : 1;
}

static bool test_comparator(const Version &v, std::string token)
{
  std::string op;
  if (token.compare(0, 2, ">=") == 0 || token.compare(0, 2, "<=") == 0)
  { op= token.substr(0, 2); token.erase(0, 2); }
  else if (!token.empty() && (token[0] == '>' || token[0] == '<' || token[0] == '='))
  { op= token.substr(0, 1); token.erase(0, 1); }
  Version target;
  if (!parse(token.data(), token.size(), &target, true)) return false;
  int c= compare(v, target);
  return op.empty() || op == "=" ? c == 0 : op == ">" ? c > 0 :
         op == ">=" ? c >= 0 : op == "<" ? c < 0 : c <= 0;
}

static bool simple_range(const Version &v, const std::string &clause)
{
  std::istringstream in(clause);
  std::string token;
  bool any= false;
  while (in >> token)
  {
    any= true;
    if (token == "*" || token == "x" || token == "X") continue;
    char mode= token[0];
    if (mode == '^' || mode == '~')
    {
      token.erase(0, 1); Version low;
      if (!parse(token.data(), token.size(), &low, true) || compare(v, low) < 0) return false;
      Version high= low; high.prerelease.clear(); high.build.clear();
      if (mode == '~') { if (high.minor == UINT64_MAX) return false; ++high.minor; high.patch= 0; }
      else if (high.major) { if (high.major == UINT64_MAX) return false; ++high.major; high.minor= high.patch= 0; }
      else if (high.minor) { if (high.minor == UINT64_MAX) return false; ++high.minor; high.patch= 0; }
      else { if (high.patch == UINT64_MAX) return false; ++high.patch; }
      if (compare(v, high) >= 0) return false;
      continue;
    }
    size_t wildcard= token.find_first_of("xX*");
    if (wildcard != std::string::npos)
    {
      std::string prefix= token.substr(0, wildcard);
      if (!prefix.empty() && prefix.back() == '.') prefix.pop_back();
      std::istringstream parts(prefix); std::string p;
      uint64_t nums[2]= {0,0}; unsigned count= 0;
      while (std::getline(parts, p, '.') && count < 2)
        if (!number(p, &nums[count++])) return false;
      if (count > 0 && v.major != nums[0]) return false;
      if (count > 1 && v.minor != nums[1]) return false;
      continue;
    }
    if (!test_comparator(v, token)) return false;
  }
  return any;
}

bool satisfies(const Version &v, const char *range, size_t length)
{
  if (!range || !length || length > MAX_RANGE_LENGTH) return false;
  std::string all(range, length); size_t start= 0;
  while (start <= all.size())
  {
    size_t end= all.find("||", start);
    if (end == std::string::npos) end= all.size();
    if (simple_range(v, all.substr(start, end - start))) return true;
    if (end == all.size()) break;
    start= end + 2;
  }
  return false;
}

bool bump(const Version &v, unsigned component, std::string *out)
{
  uint64_t major= v.major, minor= v.minor, patch= v.patch;
  uint64_t *part= component == 0 ? &major : component == 1 ? &minor : &patch;
  if (*part == UINT64_MAX) return false;
  ++*part;
  if (component < 2) patch= 0;
  if (component < 1) minor= 0;
  std::ostringstream text; text << major << '.' << minor << '.' << patch;
  *out= text.str(); return true;
}

}
