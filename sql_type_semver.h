#ifndef SQL_TYPE_SEMVER_INCLUDED
#define SQL_TYPE_SEMVER_INCLUDED

/* Copyright (c) 2019,2024,2025,2026 MariaDB Corporation
   Copyright (c) 2026 lefred (Frédéric Descamps)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License. */

#include "sql_type_fixedbin_storage.h"
#include "semver.h"

/*
  Storage layout: a fixed-size precedence key, followed by the canonical
  text value and a trailing NUL. The text region's size is tied to
  semver::MAX_VERSION_LENGTH so that the parser's length cap and the
  on-disk buffer layout cannot silently drift out of sync.
*/
constexpr size_t SEMVER_PRECEDENCE_KEY_LEN= 576;
constexpr size_t SEMVER_STORAGE_LEN=
  SEMVER_PRECEDENCE_KEY_LEN + semver::MAX_VERSION_LENGTH + 1;

class Semver_storage: public FixedBinTypeStorage<SEMVER_STORAGE_LEN,
                                                  semver::MAX_VERSION_LENGTH>
{
public:
  using FixedBinTypeStorage::FixedBinTypeStorage;
  bool ascii_to_fbt(const char *str, size_t length);
  size_t to_string(char *dst, size_t dstsize) const;
  static const Name &default_value();
};

#include "sql_type_fixedbin.h"
typedef Type_handler_fbt<Semver_storage> Type_handler_semver;

#endif
