# UNA SDK — Local Patches to Vendored (Third-Party) Code

This document tracks deliberate modifications to files under `ThirdParty/` that
diverge from the upstream vendor drop. These are **not** part of the SDK's own
source: when a vendored component is re-imported at a new version, the drop will
overwrite these files and each patch below must be **re-applied**.

> This is distinct from the SDK's **own** source (e.g. the simulator OS shims
> documented in [`Simulator.md`](Simulator.md#linux-gcc)): changes to our own code
> survive a vendor bump; the patches here do not.

## How to use this document

- **Before** bumping a vendored component, read its entry so you know what local
  divergence exists.
- **After** re-importing the vendor drop, re-apply each patch that is still
  relevant (the upstream bug may have been fixed — verify), then update the
  version, PR references, and status in the table below.

---

## Vendored components

| Component | Version | Location |
|-----------|---------|----------|
| TouchGFX  | 4.26.1 (`TOUCHGFX_VERSION 42601`) | `ThirdParty/touchgfx/` |

---

## Patches

### TouchGFX 4.26.1

| # | Area | Files | PR |
|---|------|-------|----|
| 1 | Linux converter execute bit | `framework/tools/fontconvert/build/linux/fontconvert.out`, `framework/tools/imageconvert/build/linux/imageconvert.out` | #186 |
| 2 | `textconvert` Ruby 3.1/3.2 compatibility | `framework/tools/textconvert/main.rb`, `.../lib/xml_validator.rb`, `.../lib/emitters/*.rb` (24 files) | #189 |

#### 1 — Linux converter execute bit (#186)

The prebuilt Linux asset converters ship without the execute bit, so `make assets`
fails on Linux/macOS until they are `chmod`'d by hand.

- **Change:** file mode `100644` → `100755` on the two `build/linux/*.out` binaries.
- **Blobs are unchanged** — mode only.
- **Re-apply:** `chmod +x` the two Linux `.out` binaries after a re-import. (No
  action needed for the Windows binaries under `build/win/` — the execute bit is a
  git no-op on Windows checkouts.)

#### 2 — `textconvert` Ruby 3.1/3.2 compatibility (#189)

TouchGFX 4.26.1 still uses Ruby APIs that were removed in supported Ruby releases,
so the text converter crashes on any modern Ruby:

- `File.exists?` / `File::exists?` → **`File.exist?`** — the `exists?` alias was
  removed in Ruby 3.2.
- `ERB.new(str, 0, "<>")` → **`ERB.new(str, trim_mode: "<>")`** — ERB's positional
  `safe_level` argument was removed in Ruby 3.1. The `trim_mode` value (`"<>"`) is
  preserved, so generated output is byte-identical.

Both replacements are valid back to Ruby 2.6, so they are also safe on the Ruby
bundled with the TouchGFX environment (backward- and forward-compatible).

- **Re-apply:** if the new drop still uses the dead APIs, re-run the same
  substitutions across `framework/tools/textconvert/`. Verify none remain:
  - no `File.exists?` / `File::exists?` — `grep -rn -E 'File[.:]+exists\?' framework/tools/textconvert/`
  - every `ERB.new(...)` uses the `trim_mode:` keyword (no positional 2nd arg).

---

## Related repo-wide policy (not a vendored-source edit)

`.gitattributes` normalizes text files to LF in the repository and marks common
binary assets as binary (PR #188). This governs how the vendored TouchGFX text and
asset files are stored but does **not** modify their content. After re-importing a
vendored component, run `git add --renormalize .` so the line-ending policy is
applied consistently to the new files.
