/* Copyright (c) 2019,2024,2025,2026 MariaDB Corporation
   Copyright (c) 2026 lefred (Frédéric Descamps)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License. */

#define MYSQL_SERVER
#include "mariadb.h"
#include "sql_class.h"
#include "sql_type_semver.h"
#include "semver.h"

bool Semver_storage::ascii_to_fbt(const char *str, size_t length)
{
  semver::Version version;
  bzero(m_buffer, sizeof(m_buffer));
  if (!semver::parse(str, length, &version)) return true;

  /* Big-endian core numbers make the fixed binary value sort numerically. */
  uint64_t numbers[3]= {version.major, version.minor, version.patch};
  size_t pos= 0;
  for (unsigned n= 0; n < 3; ++n)
    for (int shift= 56; shift >= 0; shift-= 8)
      m_buffer[pos++]= static_cast<char>(numbers[n] >> shift);

  /* Numeric prerelease identifiers sort before non-numeric identifiers. */
  if (version.prerelease.empty())
    m_buffer[pos++]= 3;                 // A release sorts after a prerelease.
  else
  {
    for (const std::string &id : version.prerelease)
    {
      /*
        The length byte below can only hold 0..255. semver::parse() already
        enforces this indirectly (the whole input is capped to
        MAX_VERSION_LENGTH bytes), but that is an implicit, easily-broken
        coupling: check it explicitly here rather than let a future change
        to the cap silently truncate the encoded length.
      */
      if (id.size() > semver::MAX_VERSION_LENGTH) return true;
      bool numeric= true;
      for (char c : id) if (c < '0' || c > '9') { numeric= false; break; }
      if (numeric)
      {
        if (pos + id.size() + 2 >= SEMVER_PRECEDENCE_KEY_LEN) return true;
        m_buffer[pos++]= 1;
        m_buffer[pos++]= static_cast<char>(id.size());
        memcpy(m_buffer + pos, id.data(), id.size()); pos+= id.size();
      }
      else
      {
        if (pos + id.size() + 2 >= SEMVER_PRECEDENCE_KEY_LEN) return true;
        m_buffer[pos++]= 2;
        memcpy(m_buffer + pos, id.data(), id.size()); pos+= id.size();
        m_buffer[pos++]= 0;
      }
    }
    m_buffer[pos++]= 0;                 // A shorter identifier list sorts first.
  }
  memcpy(m_buffer + SEMVER_PRECEDENCE_KEY_LEN, version.text.data(),
        version.text.size());
  return false;
}

size_t Semver_storage::to_string(char *dst, size_t dstsize) const
{
  const char *text= m_buffer + SEMVER_PRECEDENCE_KEY_LEN;
  size_t length= strnlen(text, semver::MAX_VERSION_LENGTH + 1);
  if (length >= dstsize) return 0;
  memcpy(dst, text, length); dst[length]= '\0';
  return length;
}

const Name &Semver_storage::default_value()
{
  static Name value(STRING_WITH_LEN("0.0.0"));
  return value;
}
