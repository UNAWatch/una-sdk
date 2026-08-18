# App Configuration Fields

## Revision History

| Revision | Date of Changes | Matter of Change | Note | Editor |
|----------|-----------------|------------------|------|--------|
| 1.00     | 18.08.2026      | Creating: developer-declared configuration fields, the on-watch values file, the app-side reader, and the companion-app specification | | Ross Ryles |

## 1. Overview

An app can declare a list of **configuration fields** in its `config.json`. The companion
app asks the user to fill them in when the app is installed, writes the answers to a JSON
file next to the `.uapp` on the watch, and lets the user edit them again at any time. The
watch app reads that file when it starts, through `SDK::AppConfig`.

This gives an app the settings that are painful to enter on a 240x240 touchscreen — names,
coordinates, calibration constants, thresholds — without the app having to build an
on-watch editor for them.

The feature needs **no kernel firmware change**. It is built entirely from mechanisms that
already exist:

| Mechanism | Already provides |
|-----------|------------------|
| BLE File Transfer Service | Writing and reading arbitrary files under `/Apps/<AppDir>/` |
| `SDK::Kernel::fs` | Sandbox-rooted file access; `"/"` is the app's own directory |
| `SDK::JsonStreamReader` / `JsonStreamWriter` | coreJSON-backed parsing and serialisation |

### 1.1 What this is *not*

**Not a replacement for on-watch settings.** Config fields are set rarely, from the phone.
Anything the user changes *in the moment* — an alert distance mid-run, a timer duration —
belongs in the app's own on-watch settings. No value should live in both places.

**Not a place to re-ask for data the kernel already owns.** `SDK::Interface::ISettings`
already gives every app the user's unit system (`isUnitsImperial()`), their heart-rate zone
thresholds (`getHrThresholds()`) and their daily goals (`getDailyGoals()`), and the kernel
sends profile data such as height to the apps that need it. Declaring a config field for any
of these duplicates a value the user has already entered once, and the two copies will
eventually disagree.

**Not secure storage.** The values file is plaintext on a FAT volume that is readable over
USB mass storage and over BLE. Do not use config fields for passwords, API tokens or
anything else whose disclosure matters. That is why there is no `secret` field type.

**Not a runtime channel.** A change made on the phone is picked up the **next time the app
starts** (section 7.2). It is not a live data feed.

### 1.2 Data flow

```
   config.json                             Companion app (phone)
   +---------------------+                 +----------------------------+
   | "configFile"        |  1. read the    |                            |
   | "configFields": [   |---- package --->| 2. prompt the user         |
   |    {...}, {...}     |                 | 3. validate the input      |
   | ]                   |                 |                            |
   +---------------------+                 +-------------+--------------+
   (stays on the phone;                                  |
    never copied to the watch)                4. FTS WRITE offset 0
                                                         |
                                                         v
                              2:/Apps/<AppDir>/<configFile>
                              { "schema": 1, "values": { ... } }
                                                         |
                                            5. read once at launch
                                                         v
                                             +-------------------------+
                                             | SDK::AppConfig          |
                                             | (linked into the .uapp) |
                                             +-----------+-------------+
                                                         |
                                        6. optional write-back (section 6)
                                                         |
                                                         +--> same file
```

`config.json` itself is **never** uploaded to the watch — that rule is unchanged (see
[App Config JSON](app-config-json.md)). The values file is a separate artifact the companion
app generates.

## 2. Declaring fields in `config.json`

Two new keys. Both are optional; an app that wants no configuration declares neither.

### 2.1 `configFile`

The name of the values file the companion app creates in the app's directory on the watch.

```json
"configFile": "app_config.json"
```

| Rule | Detail |
|------|--------|
| Required when | `configFields` is present and non-empty |
| Forbidden when | `configFields` is absent or empty |
| Pattern | `^[A-Za-z0-9][A-Za-z0-9_.-]{0,57}\.json$` |
| No path separators | `/` and `\` are rejected, and so is `..` — the file always lands in the app's sandbox root |
| Must not be | `config.json` (reserved: that is the package metadata file, which never reaches the watch). Compared case-insensitively, because the watch's volume is |
| Length | <= 63 characters, which the pattern above already enforces. `SDK::AppConfig` refuses a longer name and falls back to defaults |

Restricting this to a bare filename is deliberate. The app's sandbox root is the one
directory both the companion app and `SDK::AppConfig` can always resolve without extra
rules, and it stops a malformed or hostile package from reaching outside the app's own
directory.

### 2.2 `configFields`

An ordered array of field definitions. **Array order is display order** in the companion
app.

```json
"configFields": [
  { "id": "waypointName",   "type": "string", ... },
  { "id": "targetLatitude", "type": "float",  ... }
]
```

At most **32** fields (section 8).

### 2.3 Attributes common to every field

| Attribute | Required | Type | Rules |
|-----------|----------|------|-------|
| `id` | yes | string | `^[a-z][A-Za-z0-9_]{0,31}$`. Unique within the app, compared **case-insensitively**: `waypointName` and `waypointname` may not both be declared, because two ids that differ only in case are a bug waiting to happen. This is the key in the values file and the string the app passes to `SDK::AppConfig`. Choose it once and never change it — changing an `id` discards the user's value (section 7.3). |
| `type` | yes | string | One of `"string"`, `"bool"`, `"int"`, `"float"`. |
| `label` | yes | string | Short human-readable name, <= 32 characters. Shown next to the input. Do not put units in the label — use `unit`. |
| `description` | yes | string | Longer human-readable explanation, <= 200 characters. Shown under or beside the input. Write it for someone who has never used the app. |
| `default` | yes | matches `type` | The value used when the user has not set one. The companion app pre-fills the input with it, and the app falls back to it when the key is absent. It must itself satisfy every constraint declared on the field. |
| `required` | no (default `false`) | bool | `true` means the user must supply a value before the install can complete. The input is still pre-filled from `default`, and accepting that pre-filled value counts as supplying one. |
| `validationMessage` | no | string | <= 120 characters. Shown by the companion app when the entered value fails **any** of this field's constraints. Write it as the rule rather than as a complaint: `"Up to 16 letters, digits and spaces."` If omitted, the companion app shows its own per-constraint default message. |

`default` is mandatory on purpose: it gives the companion app something to pre-fill, gives
the app a defined fallback, and — because the app's field table repeats it (section 5.1) —
gives CI something to cross-check.

One further attribute, `unit`, applies only to the numeric types and is described with them:
a unit on a name or a switch has nothing to label.

### 2.4 Attributes by type

#### `bool`

No extra attributes. `default` is `true` or `false`.

```json
{
  "id": "vibrateOnArrival",
  "type": "bool",
  "label": "Vibrate on arrival",
  "description": "Buzz the watch once when you come within the arrival radius of the target.",
  "default": true
}
```

#### `int`

| Attribute | Required | Rules |
|-----------|----------|-------|
| `min` | yes | Integer. Inclusive lower bound. |
| `max` | yes | Integer. Inclusive upper bound, `>= min`. |
| `unit` | no | Presentation-only suffix rendered next to the input, <= 8 characters (`"m"`, `"bpm"`, `"%"`). Never part of the stored value. |

Both bounds are mandatory. A range is what makes the companion app's validation and the
app's clamping (section 5.2) well-defined, and an unbounded integer field is usually one the
developer has not finished thinking about. Values are 32-bit signed:
`-2147483648 .. 2147483647`.

```json
{
  "id": "arrivalRadiusM",
  "type": "int",
  "label": "Arrival radius",
  "description": "How close you need to get before the watch counts you as arrived.",
  "default": 25,
  "min": 5,
  "max": 500,
  "unit": "m"
}
```

#### `float`

| Attribute | Required | Rules |
|-----------|----------|-------|
| `min` | yes | Number. Inclusive lower bound. |
| `max` | yes | Number. Inclusive upper bound, `>= min`. |
| `unit` | no | Presentation-only suffix rendered next to the input, <= 8 characters (`"deg"`, `"kg"`). Never part of the stored value. |

Values are **single-precision** (IEEE-754 binary32) on the watch, matching the rest of the
platform: the GPS sensor layer reports latitude, longitude and altitude as `float`, and
`SDK::TrackMap::GpsPoint` stores them as `float`. A config value therefore has exactly the
same resolution as the sensor data it is compared against. Near +/-180 degrees that is
roughly 1-2 m of quantisation on a coordinate, which is well inside GPS error. Do not design
for more precision than that.

```json
{
  "id": "targetLongitude",
  "type": "float",
  "label": "Target longitude",
  "description": "Longitude of the waypoint in decimal degrees. East is positive.",
  "default": -0.1276,
  "min": -180.0,
  "max": 180.0,
  "unit": "deg"
}
```

#### `string`

| Attribute | Required | Rules |
|-----------|----------|-------|
| `maxLength` | yes | 1 .. 128. Length in **UTF-8 bytes**, not characters. |
| `minLength` | no (default `0`) | 0 .. `maxLength`. A value `>= 1` makes an empty string invalid. |
| `pattern` | no | Regular expression, <= 256 characters. See section 3.2. |

`maxLength` is mandatory because the app has to size a buffer for the value, and it is
counted in bytes because that is the unit both the buffer and the file-size limit are
measured in. A companion app must count bytes, not characters: a 16-byte field holds 16
ASCII characters but only 4 four-byte emoji.

```json
{
  "id": "waypointName",
  "type": "string",
  "label": "Waypoint name",
  "description": "Shown on the watch while you navigate. Letters, digits and spaces only.",
  "default": "Waypoint",
  "minLength": 1,
  "maxLength": 16,
  "pattern": "[A-Za-z0-9 ]+",
  "validationMessage": "Up to 16 letters, digits and spaces."
}
```

### 2.5 Complete example

```json
{
  "type": ["utility"],
  "name": "Waypoint",
  "icon": "Resources/icon_60x60.png",
  "binary": "Waypoint_1.0.0.uapp",
  "appVersion": "1.0.0",
  "minKernelVersion": "1.4.0",
  "requiredHardware": ["GPS"],
  "description": "Navigate back to a point you saved earlier.",
  "configFile": "app_config.json",
  "configFields": [
    {
      "id": "waypointName",
      "type": "string",
      "label": "Waypoint name",
      "description": "Shown on the watch while you navigate. Letters, digits and spaces only.",
      "default": "Waypoint",
      "minLength": 1,
      "maxLength": 16,
      "pattern": "[A-Za-z0-9 ]+",
      "validationMessage": "Up to 16 letters, digits and spaces."
    },
    {
      "id": "targetLatitude",
      "type": "float",
      "label": "Target latitude",
      "description": "Latitude of the waypoint in decimal degrees. North is positive.",
      "default": 51.5072,
      "min": -90.0,
      "max": 90.0,
      "unit": "deg",
      "required": true
    },
    {
      "id": "targetLongitude",
      "type": "float",
      "label": "Target longitude",
      "description": "Longitude of the waypoint in decimal degrees. East is positive.",
      "default": -0.1276,
      "min": -180.0,
      "max": 180.0,
      "unit": "deg",
      "required": true
    },
    {
      "id": "arrivalRadiusM",
      "type": "int",
      "label": "Arrival radius",
      "description": "How close you need to get before the watch counts you as arrived.",
      "default": 25,
      "min": 5,
      "max": 500,
      "unit": "m"
    },
    {
      "id": "vibrateOnArrival",
      "type": "bool",
      "label": "Vibrate on arrival",
      "description": "Buzz the watch once when you come within the arrival radius.",
      "default": true
    }
  ]
}
```

## 3. Validation

Validation happens in the companion app, before anything is written. The watch app never
sees an invalid value that the companion app produced — but it still defends itself
(section 5.3), because a values file can also arrive from a hand edit over USB, from an
older or newer build of the companion app, or from a downgraded `.uapp`.

### 3.1 Order of checks

Checks run in this order, and the **first** failure is what gets reported:

1. **Type** — the entered value is of the declared type.
2. **Required** — a `required` field is not empty.
3. **Length** — `minLength` / `maxLength`, in UTF-8 bytes.
4. **Range** — `min` / `max`, inclusive.
5. **Pattern** — the regular expression, as a full match.

A fixed order means the same bad input always produces the same message, on both platforms.

### 3.2 The `pattern` dialect

A pattern must behave identically on iOS and Android, so the dialect is a deliberately small
subset with no constructs whose semantics or performance differ between engines.

**Matching is an implicit full match.** The pattern must match the **entire** value. Do not
write `^` and `$` — they are unnecessary. Implementations MUST anchor the match themselves:

| Platform | Recipe |
|----------|--------|
| JavaScript | `new RegExp('^(?:' + pattern + ')$').test(value)` |
| Swift / `NSRegularExpression` | `firstMatch(...)` and require the match range to equal the whole string |
| Java / Kotlin | `Pattern.compile(pattern).matcher(value).matches()` — already a full match |

The `(?:...)` wrapper in the JavaScript recipe is not optional: without it, a top-level
alternation such as `abc|def` would bind as `^abc` or `def$` and let unintended values
through.

**Allowed:** literal characters; `.`; character classes `[...]` including ranges and
negation; the letter escapes `\d \D \w \W \s \S \b \B \n \r \t` and backslash-escaped
punctuation (`\. \[ \] \( \) \{ \} \| \+ \* \? \- \/ \\`); groups `(...)` and `(?:...)`;
alternation `|`; the quantifiers `* + ? {n} {n,} {n,m}` and their lazy forms `*? +? ??`.

The escape list is a **whitelist**: any other letter after a backslash is refused. `\A` and
`\Z` are the reason — they are anchors in Python, Java and ICU but *literal letters* in
JavaScript, so a pattern using them matches different strings on iOS and Android.

**Forbidden:** backreferences (`\1`, `\k<name>`); lookahead and lookbehind (`(?=`, `(?!`,
`(?<=`, `(?<!`); named capture groups; inline flags such as `(?i)`; atomic groups and
possessive quantifiers; Unicode property escapes (`\p{...}`); and the anchors `^` and `$`,
which are redundant because matching is already a full match. (A `^` immediately inside a
character class is negation, not an anchor, and remains legal: `[^0-9]`.)

Two more constructs are refused for the same cross-engine reason:

- **`]` immediately after `[`.** `[]]` is a class containing `]` in Python and Java, but an
  empty class followed by a literal `]` in JavaScript. Write `[\]]`.
- **A brace that is not a complete quantifier.** `a{,3}` is a quantifier in Python, literal
  text in JavaScript and a syntax error in Java. Write `a{0,3}`, or `a\{,3\}` for literal
  braces.

Every construct that survives means the same thing in all three engines, and the subset is
expressible in a linear-time engine, so an implementation *may* use one.

**Nested unbounded quantifiers are also rejected.** A quantified group whose body is itself
unbounded — `(a+)+`, `(a*)*`, `(\d+)*`, and the same shapes wrapped in extra parentheses like
`((a+))+` — backtracks exponentially in the backtracking engines iOS and Android use, with no
backreference or lookaround involved. A 128-byte input is more than enough to freeze a phone
on one of those, so `validate_app_config.py` refuses the shape and the developer rewrites it
(`(a+)+` is just `a+`).

That check is a safety net, not a proof: other shapes backtrack badly too — `(a|a)+` and
`(a?)*` among them — and detecting them all is not something a linter can promise. Keep
patterns simple, and prefer `minLength`/`maxLength` over clever expressions.

Patterns are **case-sensitive** and are applied to the value exactly as entered. The
companion app must not trim whitespace — if leading or trailing spaces should be rejected,
the pattern must say so.

### 3.3 Error messages

A field's `validationMessage` is shown for **any** failure on that field. One message per
field is enough in practice, because a good message states the whole rule
(`"Up to 16 letters, digits and spaces."`) rather than one constraint at a time.

When `validationMessage` is absent, the companion app supplies its own message for the
constraint that failed (section 9.5), so a field is never reported with a bare "invalid".

## 4. The values file

### 4.1 Location and name

```
2:/Apps/<AppDir>/<configFile>
```

`<AppDir>` is the app's directory on the watch — the one the companion app created when it
transferred the `.uapp`. Over BLE FTS the same path is `/Apps/<AppDir>/<configFile>`,
relative to the file-transfer root. From inside the app, the sandbox makes it simply
`/<configFile>`.

### 4.2 Envelope

```json
{
  "schema": 1,
  "values": {
    "waypointName": "Trailhead",
    "targetLatitude": 51.5072,
    "targetLongitude": -0.1276,
    "arrivalRadiusM": 25
  }
}
```

| Key | Rules |
|-----|-------|
| `schema` | Integer, currently `1`. Identifies the envelope format, not the app's field set. |
| `values` | Object. Keys are field `id`s; values are the user's answers. |

A reader that finds any `schema` other than `1` must ignore the whole file and use its
defaults, rather than guess at a format it was not built for. This matches how
`SDK::Variant::Config` treats an unknown config schema.

The file is **strict JSON**: UTF-8, no BOM, no comments, no trailing commas. (The examples in
[App Config JSON](app-config-json.md) carry `//` comments for the reader's benefit; real
files must not.)

### 4.3 Value encoding

| Declared type | JSON encoding | Reader behaviour |
|---------------|---------------|------------------|
| `string` | JSON string | Longer than `maxLength` bytes: truncated at a UTF-8 boundary. |
| `bool` | `true` / `false` | A number or string here is invalid. |
| `int` | JSON integer, no fraction and no exponent | A fractional number is invalid, not rounded. |
| `float` | JSON number | An integer literal is accepted (`51` means `51.0`). |

A **reader** must accept any JSON number, exponent notation included, up to **64 characters**
of number text; anything longer is treated as not set. A **writer** must stay inside that
limit. The companion app should emit plain decimal, because its values come from a numeric
keypad and people read this file: no exponent, no thousands separators, `.` as the decimal
separator, and no fractional part for an `int`.

`SDK::AppConfig` is less tidy when the *watch* writes a value back: it formats with 9
significant digits, which yields exponent form for magnitudes below `1e-4` or at `1e9` and
above (`0.00005` is written `4.99999987e-05`). That is valid JSON and every reader must cope
with it, but it means a companion app cannot assume the file it reads back contains only the
plain decimals it wrote.

`null`, a wrong JSON type, or a malformed number means the key is treated as **not set**:
the app falls back to that field's default. A well-formed number outside the field's
`min`..`max` is **clamped**, not discarded (section 5.2) — a value the user plainly meant
should not silently disappear.

The two are distinguished by representability, not by range: a number a `float` field cannot
hold at all (`1e40`, which overflows binary32 to infinity) counts as malformed and falls back
to the default, whereas `120.0` in a `-90`..`90` field is real and gets clamped to `90`. The
same applies to an `int` field and a value outside 32-bit range.

**Control characters must not appear in string values.** No phone keyboard produces them in a
single-line field, and they cost six bytes each once escaped, which is why the 8 KB budget in
section 8 assumes their absence. `SDK::AppConfig` still escapes them correctly if a hand-edited
file contains some.

### 4.4 Which keys are present

The companion app writes the file **whenever the app declares any config fields**, so its
presence is predictable. It contains only the ids the user actually set, so **key presence
is not** predictable:

- A `required` field is always present, because the user cannot finish the install without
  it.
- An optional field the user left alone is **absent**, and the app uses its default.
- An optional field the user set and then cleared is **removed** from `values`, which is how
  a value is reset to the app's default.
- `values` may legitimately be an empty object `{}`.

An app must therefore never treat a missing key as an error, and must never require the file
to exist at all — a user can delete it over USB.

### 4.5 Unknown keys

A reader must ignore keys it does not recognise, and a writer must preserve them
(section 6.2). This is what makes a downgrade survivable: an older `.uapp` that knows fewer
fields does not destroy the values belonging to the newer one.

This applies to unrecognised keys **inside `values`**. `SDK::AppConfig` rewrites the envelope
itself, so anything a companion app adds alongside `schema` and `values` is dropped the first
time the watch saves. Keep companion metadata out of this file.

## 5. Reading configuration in the app

### 5.1 Declare the field table once

`config.json` never reaches the watch, so the app carries its own copy of the field
contract — one `constexpr` table, in one place:

```cpp
#include "SDK/AppConfig/AppConfig.hpp"

namespace {

using SDK::AppConfig;

constexpr AppConfig::Field kFields[] = {
    AppConfig::stringField("waypointName",     "Waypoint", 1, 16),
    AppConfig::floatField ("targetLatitude",   51.5072f, -90.0f,  90.0f),
    AppConfig::floatField ("targetLongitude",  -0.1276f, -180.0f, 180.0f),
    AppConfig::intField   ("arrivalRadiusM",   25, 5, 500),
    AppConfig::boolField  ("vibrateOnArrival", true),
};

} // namespace
```

Each entry repeats what `config.json` declares: the id, the default, and the bounds. That
duplication is intentional and is **checked by CI** — `validate_app_config.py --check-bounds`
compares this table against `config.json` and fails the build if they disagree
(section 10). Keeping the bounds in the binary is what lets the app clamp a value it should
never have received.

The table is a single greppable declaration rather than bounds scattered through call sites,
so the check does not depend on how the calls happen to be formatted. Two rules keep it
checkable: use **plain literals** (a named constant or an expression cannot be compared
against JSON), and keep **preprocessor conditionals out of the table** — the checker reads the
source as text, so an entry inside an `#if` counts as present whether or not it compiles.

### 5.2 Read the values

```cpp
Waypoint::Waypoint(const SDK::Kernel &kernel)
{
    SDK::AppConfig cfg(kernel, "app_config.json", kFields);

    cfg.getString("waypointName", mName, sizeof(mName));
    mTargetLat = cfg.getFloat("targetLatitude");
    mTargetLon = cfg.getFloat("targetLongitude");
    mRadiusM   = cfg.getInt("arrivalRadiusM");
    mVibrate   = cfg.getBool("vibrateOnArrival");
}
```

| Method | Returns |
|--------|---------|
| `bool getBool(const char *id) const` | The stored value, or the field's default. |
| `int32_t getInt(const char *id) const` | The stored value **clamped** to `[min, max]`, or the default. |
| `float getFloat(const char *id) const` | The stored value **clamped** to `[min, max]`, or the default. Non-finite values (NaN, infinity) are rejected as malformed and yield the default. |
| `size_t getString(const char *id, char *out, size_t outSize) const` | Copies a NUL-terminated value into `out`, truncated at a UTF-8 boundary to fit both `maxLength` and `outSize`. Returns the byte length written. |
| `bool has(const char *id) const` | `true` if the id was present in the file — use it to tell "the user chose this" from "this is the default". |
| `bool isLoaded() const` | `true` if the file was found, parsed, and had a supported `schema`. |

Reading an id that is not in the table is a programming error: the getter returns a
zero/empty value and logs a warning. There is no way to read an undeclared field.

### 5.3 What happens when things go wrong

Nothing in this path may prevent an app from starting. A config problem must degrade to
defaults, never to a failed launch:

| Condition | Result |
|-----------|--------|
| File absent | All defaults. `isLoaded()` is `false`. |
| File unreadable, or larger than 8 KB | All defaults. |
| Not valid JSON | All defaults. |
| `schema` missing or not `1` | All defaults. |
| `values` missing or not an object | All defaults. |
| One key of the wrong type, or `null` | That field only falls back to its default. |
| One numeric key out of range | That field only is clamped to its declared bounds. |
| One string longer than `maxLength` | That field only is truncated at a UTF-8 boundary. |

### 5.4 In the simulator

The host simulator's app sandbox is a directory on your PC, so testing configuration means
dropping a file into it — no phone required:

```
<simulator working directory>/app_config.json
```

Edit it, restart the simulator, and the app picks the values up exactly as it would on the
watch. Committing a sample file alongside the app is a good way to document what the
configuration is supposed to look like.

## 6. Writing configuration from the app

An app may write the values file, so a value that is convenient to capture on the watch —
"set the target to where I am standing" — does not have to be typed into a phone.

### 6.1 API

```cpp
cfg.setFloat("targetLatitude",  fix.latitude);
cfg.setFloat("targetLongitude", fix.longitude);

if (!cfg.save()) {
    LOG_WARN("Could not save waypoint\n");
}
```

| Method | Behaviour |
|--------|-----------|
| `bool setBool/setInt/setFloat(const char *id, T value)` | Validates against the field table, clamping a numeric value into range. Updates the in-memory value and marks the file dirty. Returns `false` for an unknown id. |
| `bool setString(const char *id, const char *value)` | As above, truncating to `maxLength` at a UTF-8 boundary. |
| `bool save()` | Writes the file if anything is dirty. Returns `false` if the write failed. |
| `bool clear(const char *id)` | Removes the id from the file, so the field falls back to its default on the next launch. |

A setter changes what the app sees immediately; `save()` is what makes it survive a restart.

### 6.2 How `save()` writes

`save()` never truncates the live file in place, because a reset or a flat battery halfway
through would leave the app with a half-written config and no way back:

1. Write the complete new document to `<configFile>.tmp`.
2. Flush and close it.
3. `remove(<configFile>)`.
4. `rename(<configFile>.tmp, <configFile>)`.

The document it writes contains:

- every id the app explicitly set, or that was already present in the file with a usable
  value — **not** every declared id, so the distinction between "the user chose this" and
  "this is the default" is preserved. A key that was present but unusable (wrong type, `null`,
  a malformed number) is dropped, which quietly cleans up a corrupt entry;
- every unrecognised key from the previous file, copied through unchanged (section 4.5).

### 6.3 Recovering an interrupted write

If a reset lands between steps 3 and 4, the values file is missing but `<configFile>.tmp` is
intact. On load, `SDK::AppConfig` detects exactly that — no values file, a parseable
temporary file — and completes the rename before reading. A leftover temporary file that
does not parse is deleted.

### 6.4 Both sides can write: the conflict rule

The file has two writers, so the rule is simply **last writer wins**, with one obligation on
the companion app: it MUST re-read the file over FTS immediately before showing its edit
screen, so what it displays is what is actually on the watch.

That leaves one narrow race: the user has the phone's edit screen open *while* the app
changes the same file on the watch. Whoever saves first loses their change. This is
accepted — it needs two simultaneous edits of the same app's configuration on two devices —
and it is deliberately not defended with revision counters or locking, which would cost far
more complexity than the case is worth.

A companion app that wants to notice it anyway can do so for free on FTS version 5 or later
(`DIGEST` must be gated on that version — see
[BLE File Transfer Service](BLE-File-Transfer-Service.md)): `DIGEST` returns a CRC-32
and size for the file, so comparing them against what it last wrote reveals that the watch
changed the file, without transferring it again.

## 7. Lifecycle

### 7.1 Install

1. The companion app reads `config.json` from the package.
2. If `configFields` is non-empty, it shows the configuration screen, pre-filled from each
   field's `default`. Required fields must be satisfied before the install can continue.
3. The `.uapp` is transferred, which creates the app's directory.
4. The values file is written, containing the ids the user set.

Configuration is collected *before* the transfer so that abandoning the form costs nothing,
and written *after* it because the directory has to exist first.

### 7.2 Editing later

The user can edit the configuration at any time. The companion app re-reads the file first
(section 6.4), shows the current values, and writes the whole file back from offset 0 — an
FTS write at offset 0 truncates, so a shorter document cannot leave stale bytes behind.

Because the app reads its configuration at launch, the companion app must tell the user that
changes take effect the **next time they open the app**. An app that is running when the
file changes carries on with the values it started with.

### 7.3 Updating the app

An update may add, remove or re-specify fields. The companion app merges, then always shows
the configuration screen so the user sees the result:

| Case | Behaviour |
|------|-----------|
| Id still declared, same type, value still valid | Kept. |
| Id still declared, but the value now fails validation | Reset to the new `default`, and the field is flagged in the UI so the user sees it changed. |
| Id no longer declared | Dropped. |
| Id declared with a different `type` | Treated as a new field: the old value is dropped. |
| Newly declared id | Pre-filled from its `default`. |

Then the user confirms, and the merged file is written. This is why an `id` must be stable
across versions: renaming one is indistinguishable from deleting a field and adding another,
and the user's value is lost.

### 7.4 Uninstall

Uninstalling deletes the app's directory, and the values file with it. Nothing is retained,
so a later reinstall starts from the app's defaults.

## 8. Limits

These are hard limits. Tooling rejects anything past them, and `SDK::AppConfig` is sized for
them.

| Limit | Value | Why |
|-------|-------|-----|
| Fields per app | 32 | Keeps the app-side state to a fixed-size table and a 32-bit presence mask. |
| `id` length | 32 characters | |
| `label` length | 32 characters | Fits a phone form row. |
| `description` length | 200 characters | |
| `validationMessage` length | 120 characters | |
| `unit` length | 8 characters | |
| `maxLength` ceiling for a string field | 128 bytes | Bounds the app's buffers. |
| `pattern` length | 256 characters | |
| `configFile` length | 63 characters | `SDK::AppConfig` refuses a longer name and uses defaults. |
| Number text in the values file | 64 characters | Longer number text is treated as not set. |
| Values file size | 8192 bytes | The reader holds the whole file; matches the existing variant-config cap. |
| `int` range | -2147483648 .. 2147483647 | 32-bit signed. |
| `float` precision | IEEE-754 binary32 | Matches the platform's GPS and track types. |

## 9. Companion app specification

This section is normative for the mobile implementation.

### 9.1 Reading the declaration

Parse `configFields` from the package's `config.json`. Validate it against
`Utilities/Scripts/app_packer/app-config.schema.json` and refuse to install a package that
does not conform — a malformed declaration is a broken package, not something to work
around. Render fields in array order.

### 9.2 Install flow

As section 7.1. A field with `required: true` blocks the install until it is satisfied;
every other field may be left at its default. Write the values file only after the `.uapp`
transfer has completed, to `/Apps/<AppDir>/<configFile>`, with a single `WRITE` at
offset 0. On FTS version 5 or later, a `DIGEST` afterwards is cheap insurance that the file
arrived intact; on earlier versions it is unavailable and must not be sent.

### 9.3 Edit flow

1. Read `/Apps/<AppDir>/<configFile>` from the watch.
2. If it is missing or unparseable, fall back to all defaults rather than showing an error —
   the user's remedy is the same either way.
3. Pre-fill each field from `values` if the key is present, otherwise from `default`, and
   indicate which fields are still at their default.
4. Offer a per-field "reset to default" action, which removes the key from `values` rather
   than writing the default value.
5. **Emptying an optional string field removes its key too** — do not write `""`. Otherwise
   two companion apps would represent "the user cleared this" differently, and the watch can
   tell them apart: an absent key means the app's default, while `""` is a real value that
   fails any `minLength` of 1 or more. A `required` field cannot be emptied.
5. On save, write the whole document from offset 0.
6. Tell the user the change applies the next time they open the app.

### 9.4 Input controls

| Type | Control |
|------|---------|
| `string` | Single-line text field. Show a byte counter against `maxLength`; block input past it. Plain keyboard, no autocorrect or autocapitalisation. |
| `bool` | Switch or checkbox. |
| `int` | Numeric keypad with no decimal separator. Offer a minus sign only when `min < 0`. |
| `float` | Decimal keypad. Display the decimal separator in the user's locale, but always serialise with `.` and no thousands separators or exponent. Offer a minus sign only when `min < 0`. |

Render `label` as the row's name, `description` as help text below it or behind an info
control, and `unit` immediately after the input. `unit` is decoration: never append it to
the stored value.

### 9.5 Default messages

Used when a field has no `validationMessage`. Wording is the implementation's own; these are
the cases that must be covered:

| Constraint | Case to cover |
|------------|---------------|
| required | The field is empty. |
| minLength / maxLength | Too short, too long (state the limit). |
| min / max | Below the minimum, above the maximum (state the bound). |
| pattern | The value does not match the required format. |
| type | The text is not a whole number, or not a number. |

### 9.6 Rules that must not be broken

- **Never write an invalid value.** Validation failures block the save.
- **Never write a key the app does not declare**, and never drop a key that the watch's copy
  had but this version does not declare *except* through the update merge in section 7.3.
- **Never trim** whitespace from a string value.
- **Count string length in UTF-8 bytes**, not characters.
- **Serialise numbers plainly**: no exponent, no thousands separators, `.` as the decimal
  separator, and no fractional part for an `int`.
- **Serialise strings as UTF-8**, escaping only what JSON requires (`"`, `\` and control
  characters). Do not `\uXXXX`-escape ordinary non-ASCII text: the 8 KB file limit is
  budgeted assuming you do not, and `validate_app_config.py` computes each app's worst-case
  file size on that basis.
- **Write `schema: 1`** in every file.
- **Re-read before editing** (section 6.4).

## 10. Tooling

`Utilities/Scripts/app_packer/validate_app_config.py` is the single implementation of the
rules above:

```bash
# Validate a package's declaration
python Utilities/Scripts/app_packer/validate_app_config.py --check config.json

# Cross-check the app's constexpr field table against config.json
python Utilities/Scripts/app_packer/validate_app_config.py --check config.json \
    --check-bounds Software/Libs/Sources/AppConfigFields.cpp
```

It validates `config.json` against `app-config.schema.json`, applies the rules a JSON Schema
cannot express (the regex subset, `default` satisfying its own constraints, `configFile`
naming, byte-length limits), and — with `--check-bounds` — parses the `SDK::AppConfig::Field`
table out of the given source file and compares every id, type, default and bound against
the declaration.

The apps CI runs both, so a package cannot reach the store with a declaration its own binary
disagrees with.

## 11. Worked example

The [Waypoint tutorial](Tutorials/Waypoint/ARCHITECTURE.md) is a complete utility app built
around this feature: it declares five fields covering all four types (two floats, for the
coordinate pair), navigates to the
configured coordinate, and writes the file back when the user saves their current position
as the new target.
