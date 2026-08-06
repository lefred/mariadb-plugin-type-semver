# AI Changes

## 2026-08-06 — Claude Sonnet 5 (`claude-sonnet-5`)

Following a code review against the MariaDB Server coding standards and a
security pass, fixed the two lower-severity findings that came out of that
review (the review found no exploitable bug; these are hardening fixes):

- **Unbounded `SEMVER_SATISFIES()` range argument.** `semver::satisfies()`
  had no length limit on its `range` argument, unlike version strings which
  were already capped at 255 bytes in `semver::parse()`. A caller could pass
  an arbitrarily large range expression (up to `max_allowed_packet`) and
  have it walked token-by-token with no sanity bound. Added a named
  `semver::MAX_RANGE_LENGTH` (1024 bytes) constant and a matching length
  check at the top of `satisfies()`, mirroring the existing version-length
  guard in `parse()`.
  Files: `semver.h`, `semver.cc`.

- **Implicit coupling between the parser's length cap and the on-disk
  buffer layout.** `sql_type_semver.h` hardcoded the storage buffer as
  `FixedBinTypeStorage<832, 255>`, and `sql_type_semver.cc` independently
  hardcoded the `576`-byte precedence-key offset and cast prerelease
  identifier lengths to a single `char` via `static_cast<char>(id.size())`.
  This was safe only because `semver::parse()` happened to cap all input at
  255 bytes elsewhere in a different file — nothing tied the two together,
  so a future change to either constant could silently reintroduce a
  length-byte truncation. Replaced the magic numbers with named constants
  (`semver::MAX_VERSION_LENGTH`, `SEMVER_PRECEDENCE_KEY_LEN`,
  `SEMVER_STORAGE_LEN`) derived from a single source of truth, and added an
  explicit `id.size() > semver::MAX_VERSION_LENGTH` guard before the cast in
  `ascii_to_fbt()` so the check no longer depends on an implicit, easily
  broken invariant.
  Files: `sql_type_semver.h`, `sql_type_semver.cc`.

Verified both changes with a syntax/type check of all four `.cc` files
against the MariaDB `BIN-DEBUG` build tree's include paths and compiler
flags (`g++ -std=gnu++17 -fsyntax-only`) — no errors.

