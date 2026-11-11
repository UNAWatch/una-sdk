# Texts

## Overview

The Texts widget is a technical helper container for TouchGFX. It bundles all standard text styles (typographies) and the Poppins font family used across UNA Watch applications into a single importable package. Its purpose is to pre-populate a new project with the complete set of text templates in one step, rather than creating them manually.

## Purpose

When starting a new app, import this container to instantly have all required fonts and text styles available in TouchGFX Designer. Once you have copied the needed text styles into your own screens, you can delete the Texts container along with any unused text IDs and typographies to reduce the final application size.

## Included Fonts

| File | Weight |
|------|--------|
| `Poppins-Regular.ttf` | Regular |
| `Poppins-Medium.ttf` | Medium |
| `Poppins-SemiBold.ttf` | SemiBold |
| `Poppins-Italic.ttf` | Italic |

All typographies use 4 bpp anti-aliasing and include the full printable ASCII character set plus the ellipsis character (`…`).

## Available Typographies

| Typography ID | Font | Size (px) |
|---------------|------|-----------|
| `Poppins_Regular_14` | Regular | 14 |
| `Poppins_Regular_16` | Regular | 16 |
| `Poppins_Regular_18` | Regular | 18 |
| `Poppins_Medium_10` | Medium | 10 |
| `Poppins_Medium_18` | Medium | 18 |
| `Poppins_Medium_25` | Medium | 25 |
| `Poppins_Medium_50` | Medium | 50 |
| `Poppins_SemiBold_18` | SemiBold | 18 |
| `Poppins_SemiBold_20` | SemiBold | 20 |
| `Poppins_SemiBold_25` | SemiBold | 25 |
| `Poppins_SemiBold_30` | SemiBold | 30 |
| `Poppins_SemiBold_35` | SemiBold | 35 |
| `Poppins_SemiBold_40` | SemiBold | 40 |
| `Poppins_SemiBold_60` | SemiBold | 60 |
| `Poppins_Italic_18` | Italic | 18 |
| `Poppins_Italic_20` | Italic | 20 |

## Text ID Naming Convention

Every typography is represented by **three text IDs** — one per alignment:

| Suffix | Alignment |
|--------|-----------|
| *(none)* | Center |
| `_L` | Left |
| `_R` | Right |

For example, `Poppins_SemiBold_30` provides:
- `TMP_SEMIBOLD_30` — center-aligned
- `TMP_SEMIBOLD_30_L` — left-aligned
- `TMP_SEMIBOLD_30_R` — right-aligned

This gives **48 template text IDs** in total. All use a wildcard placeholder (`<>`) so their rendered content is always supplied at runtime.

## Usage in TouchGFX Designer

1. **Import the package**:
   - Open TouchGFX Designer
   - Go to `Edit` → `Import` → `Custom Containers`
   - Select `Texts.tpkg`

2. **Place on a temporary screen** (or any screen) — the container does not need to be visible; importing it is enough to register the fonts and text IDs in the project.

3. **Use a text style** in your own TextArea components:
   - In the TextArea properties, set the **Text ID** field to the desired `TMP_*` identifier (e.g. `TMP_SEMIBOLD_40`)
   - Add a **Wildcard 1** to the TextArea and set its value — this is the actual text content that will be rendered at runtime

4. **Clean up when done**:
   - Delete the Texts custom container from your screen
   - In the Texts editor, remove any `TMP_*` text IDs that are not referenced by your own components
   - Remove unused typographies to reduce flash memory usage

## Files Included

- `manifest.xml` — package manifest (requires TouchGFX ≥ 4.26.1)
- `content/CustomContainerExport.touchgfx` — Designer container definition
- `content/CustomContainerManifest.xml` — list of texts, typographies, and fonts
- `content/texts.xml` — full text database with all 48 template IDs and 16 typographies
- `files/gui/include/gui/containers/Texts.hpp` — generated header
- `files/gui/src/containers/Texts.cpp` — generated source
- `files/assets/fonts/` — 4 Poppins TTF font files

## Dependencies

- TouchGFX Framework (minimum version 4.26.1)
- No runtime C++ dependencies — the container has no custom logic beyond the generated base class
