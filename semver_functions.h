#ifndef SEMVER_FUNCTIONS_INCLUDED
#define SEMVER_FUNCTIONS_INCLUDED

/* Copyright (c) 2019,2024,2025,2026 MariaDB Corporation
   Copyright (c) 2026 lefred (Frédéric Descamps)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License. */

#include <mysql/plugin_function.h>

extern Plugin_function plugin_descriptor_semver_major;
extern Plugin_function plugin_descriptor_semver_minor;
extern Plugin_function plugin_descriptor_semver_patch;
extern Plugin_function plugin_descriptor_semver_prerelease;
extern Plugin_function plugin_descriptor_semver_build;
extern Plugin_function plugin_descriptor_semver_is_valid;
extern Plugin_function plugin_descriptor_semver_compare;
extern Plugin_function plugin_descriptor_semver_satisfies;
extern Plugin_function plugin_descriptor_semver_normalize;
extern Plugin_function plugin_descriptor_semver_bump_major;
extern Plugin_function plugin_descriptor_semver_bump_minor;
extern Plugin_function plugin_descriptor_semver_bump_patch;
#endif
