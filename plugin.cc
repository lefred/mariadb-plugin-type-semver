/* Copyright (c) 2019,2024,2025,2026 MariaDB Corporation
   Copyright (c) 2026 lefred (Frédéric Descamps)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1335  USA */

/* MariaDB SEMVER data type and companion functions. */
#define MYSQL_SERVER
#include "mariadb.h"
#include "sql_class.h"
#include "sql_type_semver.h"
#include "semver_functions.h"
#include <mysql/plugin_data_type.h>

static struct st_mariadb_data_type plugin_descriptor_semver=
{ MariaDB_DATA_TYPE_INTERFACE_VERSION, Type_handler_semver::singleton() };

#define SEMVER_PLUGIN_ENTRY(TYPE, DESCRIPTOR, NAME, DESCRIPTION) \
{ TYPE, DESCRIPTOR, NAME, "lefred", DESCRIPTION, PLUGIN_LICENSE_GPL, \
  0, 0, 0x0100, NULL, NULL, "1.0.0", MariaDB_PLUGIN_MATURITY_BETA }

maria_declare_plugin(type_semver)
  SEMVER_PLUGIN_ENTRY(MariaDB_DATA_TYPE_PLUGIN, &plugin_descriptor_semver,
                      "semver", "Semantic Versioning 2.0.0 data type"),
  SEMVER_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN, &plugin_descriptor_semver_major,
                      "semver_major", "Return the major version"),
  SEMVER_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN, &plugin_descriptor_semver_minor,
                      "semver_minor", "Return the minor version"),
  SEMVER_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN, &plugin_descriptor_semver_patch,
                      "semver_patch", "Return the patch version"),
  SEMVER_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN, &plugin_descriptor_semver_prerelease,
                      "semver_prerelease", "Return prerelease identifiers"),
  SEMVER_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN, &plugin_descriptor_semver_build,
                      "semver_build", "Return build metadata"),
  SEMVER_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN, &plugin_descriptor_semver_is_valid,
                      "semver_is_valid", "Validate a semantic version"),
  SEMVER_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN, &plugin_descriptor_semver_compare,
                      "semver_compare", "Compare semantic version precedence"),
  SEMVER_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN, &plugin_descriptor_semver_satisfies,
                      "semver_satisfies", "Test a semantic version range"),
  SEMVER_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN, &plugin_descriptor_semver_normalize,
                      "semver_normalize", "Normalize a semantic version"),
  SEMVER_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN, &plugin_descriptor_semver_bump_major,
                      "semver_bump_major", "Increment the major version"),
  SEMVER_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN, &plugin_descriptor_semver_bump_minor,
                      "semver_bump_minor", "Increment the minor version"),
  SEMVER_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN, &plugin_descriptor_semver_bump_patch,
                      "semver_bump_patch", "Increment the patch version")
maria_declare_plugin_end;
