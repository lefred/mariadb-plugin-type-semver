/* Copyright (c) 2019,2024,2025,2026 MariaDB Corporation
   Copyright (c) 2026 lefred (Frédéric Descamps)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License. */

#define MYSQL_SERVER
#include "mariadb.h"
#include "sql_class.h"
#include "item_func.h"
#include "semver.h"
#include "semver_functions.h"

static bool version_from_item(Item *item, semver::Version *version,
                              bool loose= false)
{
  StringBuffer<256> buffer;
  String *text= item->val_str(&buffer);
  return !text || !semver::parse(text->ptr(), text->length(), version, loose);
}

enum Integer_operation { MAJOR, MINOR, PATCH, COMPARE };
class Item_func_semver_integer: public Item_int_func
{
  Integer_operation operation; LEX_CSTRING function_name;
public:
  Item_func_semver_integer(THD *thd, Item *a, Integer_operation op,
                           LEX_CSTRING name)
    : Item_int_func(thd, a), operation(op), function_name(name)
  { unsigned_flag= op != COMPARE; }
  Item_func_semver_integer(THD *thd, Item *a, Item *b, LEX_CSTRING name)
    : Item_int_func(thd, a, b), operation(COMPARE), function_name(name) {}
  longlong val_int() override
  {
    semver::Version a;
    if (version_from_item(args[0], &a)) { null_value= true; return 0; }
    null_value= false;
    if (operation == MAJOR) return static_cast<longlong>(a.major);
    if (operation == MINOR) return static_cast<longlong>(a.minor);
    if (operation == PATCH) return static_cast<longlong>(a.patch);
    semver::Version b;
    if (version_from_item(args[1], &b)) { null_value= true; return 0; }
    return semver::compare(a, b);
  }
  const Type_handler *type_handler() const override
  {
    if (unsigned_flag) return &type_handler_ulonglong;
    return &type_handler_slonglong;
  }
  LEX_CSTRING func_name_cstring() const override { return function_name; }
  Item *shallow_copy(THD *thd) const override
  { return get_item_copy<Item_func_semver_integer>(thd, this); }
};

class Item_func_semver_valid: public Item_bool_func
{
public:
  Item_func_semver_valid(THD *thd, Item *a): Item_bool_func(thd, a) {}
  bool val_bool() override
  {
    StringBuffer<256> buffer; String *text= args[0]->val_str(&buffer);
    if (!text) { null_value= true; return false; }
    semver::Version value; null_value= false;
    return semver::parse(text->ptr(), text->length(), &value);
  }
  LEX_CSTRING func_name_cstring() const override
  { return "semver_is_valid"_LEX_CSTRING; }
  Item *shallow_copy(THD *thd) const override
  { return get_item_copy<Item_func_semver_valid>(thd, this); }
};

class Item_func_semver_satisfies: public Item_bool_func
{
public:
  Item_func_semver_satisfies(THD *thd, Item *a, Item *b)
    : Item_bool_func(thd, a, b) {}
  bool val_bool() override
  {
    semver::Version version; StringBuffer<256> buffer;
    String *range= args[1]->val_str(&buffer);
    if (!range || version_from_item(args[0], &version))
    { null_value= true; return false; }
    null_value= false;
    return semver::satisfies(version, range->ptr(), range->length());
  }
  LEX_CSTRING func_name_cstring() const override
  { return "semver_satisfies"_LEX_CSTRING; }
  Item *shallow_copy(THD *thd) const override
  { return get_item_copy<Item_func_semver_satisfies>(thd, this); }
};

enum String_operation { PRERELEASE, BUILD, NORMALIZE, BUMP_MAJOR,
                        BUMP_MINOR, BUMP_PATCH };
class Item_func_semver_string: public Item_str_func
{
  String_operation operation; LEX_CSTRING function_name;
public:
  Item_func_semver_string(THD *thd, Item *a, String_operation op,
                          LEX_CSTRING name)
    : Item_str_func(thd, a), operation(op), function_name(name) {}
  bool fix_length_and_dec(THD *) override
  {
    collation.set(&my_charset_latin1); max_length= 255; set_maybe_null();
    return false;
  }
  String *val_str(String *result) override
  {
    semver::Version version;
    if (version_from_item(args[0], &version, operation == NORMALIZE))
    { null_value= true; return nullptr; }
    std::string value;
    if (operation == NORMALIZE) value= version.text;
    else if (operation == BUILD) value= version.build;
    else if (operation == PRERELEASE)
    {
      for (size_t i= 0; i < version.prerelease.size(); ++i)
      { if (i) value+= '.'; value+= version.prerelease[i]; }
    }
    else if (!semver::bump(version,
             operation == BUMP_MAJOR ? 0 : operation == BUMP_MINOR ? 1 : 2,
             &value))
    { null_value= true; return nullptr; }
    if (result->copy(value.data(), value.size(), &my_charset_latin1))
    { null_value= true; return nullptr; }
    null_value= false; return result;
  }
  LEX_CSTRING func_name_cstring() const override { return function_name; }
  Item *shallow_copy(THD *thd) const override
  { return get_item_copy<Item_func_semver_string>(thd, this); }
};

class Create_integer1: public Create_func_arg1
{
  Integer_operation operation; LEX_CSTRING name;
public: Create_integer1(Integer_operation op, LEX_CSTRING n): operation(op), name(n) {}
  Item *create_1_arg(THD *thd, Item *a) override
  { return new (thd->mem_root) Item_func_semver_integer(thd, a, operation, name); }
};
class Create_compare: public Create_func_arg2
{ public: Item *create_2_arg(THD *thd, Item *a, Item *b) override
  { return new (thd->mem_root) Item_func_semver_integer(thd, a, b,
      "semver_compare"_LEX_CSTRING); } };
class Create_valid: public Create_func_arg1
{ public: Item *create_1_arg(THD *thd, Item *a) override
  { return new (thd->mem_root) Item_func_semver_valid(thd, a); } };
class Create_satisfies: public Create_func_arg2
{ public: Item *create_2_arg(THD *thd, Item *a, Item *b) override
  { return new (thd->mem_root) Item_func_semver_satisfies(thd, a, b); } };
class Create_string1: public Create_func_arg1
{
  String_operation operation; LEX_CSTRING name;
public: Create_string1(String_operation op, LEX_CSTRING n): operation(op), name(n) {}
  Item *create_1_arg(THD *thd, Item *a) override
  { return new (thd->mem_root) Item_func_semver_string(thd, a, operation, name); }
};

static Create_integer1 create_major(MAJOR, "semver_major"_LEX_CSTRING);
static Create_integer1 create_minor(MINOR, "semver_minor"_LEX_CSTRING);
static Create_integer1 create_patch(PATCH, "semver_patch"_LEX_CSTRING);
static Create_string1 create_pre(PRERELEASE, "semver_prerelease"_LEX_CSTRING);
static Create_string1 create_build(BUILD, "semver_build"_LEX_CSTRING);
static Create_valid create_valid;
static Create_compare create_compare;
static Create_satisfies create_satisfies;
static Create_string1 create_normalize(NORMALIZE, "semver_normalize"_LEX_CSTRING);
static Create_string1 create_bump_major(BUMP_MAJOR, "semver_bump_major"_LEX_CSTRING);
static Create_string1 create_bump_minor(BUMP_MINOR, "semver_bump_minor"_LEX_CSTRING);
static Create_string1 create_bump_patch(BUMP_PATCH, "semver_bump_patch"_LEX_CSTRING);

Plugin_function plugin_descriptor_semver_major(&create_major);
Plugin_function plugin_descriptor_semver_minor(&create_minor);
Plugin_function plugin_descriptor_semver_patch(&create_patch);
Plugin_function plugin_descriptor_semver_prerelease(&create_pre);
Plugin_function plugin_descriptor_semver_build(&create_build);
Plugin_function plugin_descriptor_semver_is_valid(&create_valid);
Plugin_function plugin_descriptor_semver_compare(&create_compare);
Plugin_function plugin_descriptor_semver_satisfies(&create_satisfies);
Plugin_function plugin_descriptor_semver_normalize(&create_normalize);
Plugin_function plugin_descriptor_semver_bump_major(&create_bump_major);
Plugin_function plugin_descriptor_semver_bump_minor(&create_bump_minor);
Plugin_function plugin_descriptor_semver_bump_patch(&create_bump_patch);
