#ifndef SEMVER_INCLUDED
#define SEMVER_INCLUDED

/* Copyright (c) 2019,2024,2025,2026 MariaDB Corporation
   Copyright (c) 2026 lefred (Frédéric Descamps)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License. */

#include <string>
#include <vector>
#include <stdint.h>

namespace semver {

/* Maximum length, in bytes, of a version string accepted by parse(). */
constexpr size_t MAX_VERSION_LENGTH= 255;

/*
  Maximum length, in bytes, of a range expression accepted by satisfies().
  Ranges may chain several comparator sets ("a || b || c"), so they are
  allowed to be longer than a single version, but still bounded so that
  satisfies() cannot be made to do unbounded work on attacker-controlled
  input (e.g. a multi-megabyte range string).
*/
constexpr size_t MAX_RANGE_LENGTH= 1024;

struct Version
{
  uint64_t major, minor, patch;
  std::vector<std::string> prerelease;
  std::string build;
  std::string text;
};

bool parse(const char *str, size_t length, Version *out, bool loose= false);
int compare(const Version &a, const Version &b);
bool satisfies(const Version &version, const char *range, size_t length);
bool bump(const Version &version, unsigned component, std::string *out);

}
#endif
