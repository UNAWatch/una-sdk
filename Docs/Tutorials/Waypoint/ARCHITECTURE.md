(tutorials/waypoint/architecture)=
# Waypoint - Configuration from the Companion App

Some settings are miserable to enter with four buttons. A latitude, a name, a threshold: the
user has a phone in their pocket with a keyboard on it, and the companion app can ask them
once. Waypoint is a small utility app built around that idea. It navigates back
to a coordinate the user sets from their phone, and it can overwrite that coordinate with
wherever the user is standing.

[Project Folder](https://github.com/UNAWatch/una-sdk/tree/main/Docs/Tutorials/Waypoint)

The full contract behind this tutorial is [App Configuration Fields](../../app-config-fields.md).
This page is the worked example.

## What You'll Learn

- How to declare configuration fields in `config.json` - one of each supported type
- How the companion app turns them into a file on the watch, and where it lands
- How to read those values with `SDK::AppConfig`, and why every read has a default
- How to tell "the user chose this" from "this is our fallback"
- How to write a value back from the watch without ever corrupting the file
- How CI stops your app's idea of a field from drifting away from `config.json`

## How configuration reaches the watch

```
   config.json                     Companion app            2:/Apps/Waypoint/
   +----------------------+                                 +-------------------+
   | "configFile"         |  1. reads the package           | Waypoint.uapp     |
   | "configFields": [    |------------------------------>  | app_config.json   | <--+
   |    waypointName      |  2. asks the user               +-------------------+    |
   |    targetLatitude    |  3. validates the input                   |              |
   |    targetLongitude   |  4. writes the values file ---------------+              |
   |    arrivalRadiusM    |                                           |              |
   |    vibrateOnArrival  |                                  5. read at launch       |
   | ]                    |                                           v              |
   +----------------------+                                    SDK::AppConfig        |
   (never leaves the phone)                                           |              |
                                                6. "save my position" |              |
                                                                      +--------------+
```

`config.json` itself never reaches the watch. Only the values file does.

## Getting Started

### Prerequisites

Same as [HelloWorld](tutorials/helloworld/architecture): `UNA_SDK` set, the ST ARM GCC
toolchain on `PATH`, CMake 3.21+, and Python 3. See the [toolchain setup](toolchain-setup).

Waypoint declares `"requiredHardware": ["GPS"]`, so on hardware it needs a watch with GPS.

### Building

```bash
cd $UNA_SDK/Docs/Tutorials/Waypoint
mkdir build && cd build
cmake -G "Unix Makefiles" ../Software/Apps/Waypoint-CMake
make
```

On Windows, generate with Ninja instead - `cmake -G Ninja ../Software/Apps/Waypoint-CMake`.
The `.uapp` is produced in the build directory; the final step that copies it into `Output/`
uses a wildcard that `cmake -E copy` does not expand on Windows, so copy it across yourself
if the build stops there.

### Trying it without a phone

The simulator's app sandbox is a directory on your PC - the GUI logs its path at startup
("Path to files created by app"). Drop a file called `app_config.json` in there:

```json
{
  "schema": 1,
  "values": {
    "waypointName": "Trailhead",
    "targetLatitude": 51.5072,
    "targetLongitude": -0.1276,
    "arrivalRadiusM": 40
  }
}
```

Restart the simulator and the app reads it exactly as it would on the watch. Delete the file
and every field falls back to its declared default - which is worth trying, because that is
the path most apps get wrong.

## Declaring the fields

`Output/config.json` carries two extra keys. `configFile` names the file the companion app
will write, and `configFields` lists what to ask for. Waypoint declares five fields covering
all four supported types — two of them floats, for the coordinate pair. Abridged (see
`Output/config.json` for the full declaration, including `targetLongitude`):

```json
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
```

Points worth copying:

- **`min` and `max` are mandatory** on `int` and `float`. They are what the companion app
  validates against and what the app clamps to, and an unbounded number is usually a field
  nobody has finished thinking about.
- **`maxLength` is mandatory** on `string`, in UTF-8 **bytes**, because the app sizes a
  buffer from it.
- **`default` is mandatory** everywhere. It pre-fills the phone's input and it is what the
  app falls back to.
- **`required: true`** on the two coordinates means the install cannot finish until the user
  supplies them - a waypoint app with no waypoint is not much use.
- **`pattern`** is a deliberately small regex dialect and always matches the *whole* value,
  so no `^` or `$`. See [the dialect rules](../../app-config-fields.md).
- **`unit`** is decoration the phone renders beside the input. It is never stored in the
  value, so do not put units in `label` too.

## The field table the app carries

`config.json` stays on the phone, so the app repeats the contract in
`Software/Libs/Sources/AppConfigFields.cpp`:

```cpp
const AppConfig::Field kFields[] = {
    AppConfig::stringField("waypointName", "Waypoint", 1, 16),
    AppConfig::floatField("targetLatitude", 51.5072f, -90.0f, 90.0f),
    AppConfig::floatField("targetLongitude", -0.1276f, -180.0f, 180.0f),
    AppConfig::intField("arrivalRadiusM", 25, 5, 500),
    AppConfig::boolField("vibrateOnArrival", true),
};
```

This is duplication on purpose: it is what lets the app clamp a value it should never have
received - from a hand-edited file, a downgrade, or a buggy companion build. Duplication that
can drift is a bug waiting to happen, so CI compares the two:

```bash
python Utilities/Scripts/app_packer/validate_app_config.py \
    --check Output/config.json \
    --check-bounds Software/Libs/Sources/AppConfigFields.cpp \
    --check-bounds Software/Libs/Header/AppConfigFields.hpp
```

Change `500` to `400` in either place and the build fails with the two values side by side.
Run it yourself before opening a pull request - it also checks the whole declaration:
unique ids, `default` satisfying its own constraints, the regex dialect, and the worst-case
size of the values file.

## Reading the values

One object, constructed once, at startup:

```cpp
Service::Service(SDK::Kernel &kernel)
    : ...
    , mConfig(mKernel, WaypointConfig::kFileName,
              WaypointConfig::kFields, WaypointConfig::kFieldCount)
{
    loadConfiguration();
}

void Service::loadConfiguration()
{
    mConfig.getString("waypointName", mWaypointName, sizeof(mWaypointName));
    mTargetLatitude = mConfig.getFloat("targetLatitude");
    mTargetLongitude = mConfig.getFloat("targetLongitude");
    mArrivalRadiusM = mConfig.getInt("arrivalRadiusM");
    mVibrateOnArrival = mConfig.getBool("vibrateOnArrival");

    mTargetIsConfigured = mConfig.has("targetLatitude") &&
            mConfig.has("targetLongitude");
}
```

Note what the getters do *not* need: no error handling, no "if the file exists" branch, no
range checks. Every getter returns either the stored value, clamped to the bounds you
declared, or the field's default. That is the whole point - a configuration problem can never
stop the app from starting.

`has()` is the one thing worth thinking about. It answers "did this come from the user?",
which is different from "is it non-zero". Waypoint uses it to avoid navigating confidently
towards its own placeholder coordinate:

```cpp
if (!nav.targetIsConfigured) {
    setStatus("Set a target");
}
```

**Values are read once, at launch.** A change the user makes on their phone applies the next
time they open the app. The companion app tells them so; your app does not need to watch the
file.

### What happens when things go wrong

Every one of these degrades to defaults rather than to a crash or a failed launch:

| Condition | Result |
|-----------|--------|
| No values file at all | Every field uses its default; `isLoaded()` is `false` |
| Not valid JSON, or bigger than 8 KB | Every field uses its default |
| `"schema"` is missing or not `1` | Every field uses its default |
| One value has the wrong JSON type, or is `null` | That field alone uses its default |
| One number is outside `min`..`max` | That field alone is clamped |
| One string is longer than `maxLength` | That field alone is truncated, on a character boundary |

## Writing a value back from the watch

Configuration is not one-way. Pressing R1 stores the current position as the new target, so
the user can save a spot without typing coordinates at all:

```cpp
void Service::saveTargetHere()
{
    if (!mHasFix) {
        SDK::send_msg<CustomMessage::TargetSaved>(mKernel, false, ...);
        return;
    }

    mConfig.setFloat("targetLatitude", mLatitude);
    mConfig.setFloat("targetLongitude", mLongitude);

    const bool saved = mConfig.save();

    loadConfiguration();    // re-read, so what is in play is what is on disk
    ...
}
```

Three things `save()` does that are easy to get wrong by hand:

1. **It never truncates the live file.** The new document goes to `app_config.json.tmp`,
   which is then renamed over the original. A reset mid-write leaves the previous target
   intact, and the next launch finishes the rename.
2. **It leaves values it did not change exactly as they were** - byte for byte. Re-serialising
   `51.5072` as a float would write `51.5071983`, and the user's number would drift a little
   every time the app saved.
3. **It preserves keys it does not recognise**, so installing an older build of your app does
   not silently discard a newer version's settings.

The file has two writers now - the phone and the watch - so the rule is last writer wins, and
the companion app re-reads the file before it shows its edit screen. The only way to lose a
change is to edit on both at the same moment.

## The screen

`MainView` renders four wildcard text areas: name, distance, bearing and a status line. The
wildcard typographies (`TMP_SEMIBOLD_40` and friends) already carry the full ASCII set, so
arbitrary runtime text needs no font regeneration:

```cpp
std::snprintf(text, sizeof(text), "%d m", static_cast<int>(nav.distanceM));
touchgfx::Unicode::strncpy(distanceTextBuffer, text, DISTANCETEXT_SIZE);
distanceText.invalidate();
```

`setupScreen()` writes **every** wildcard before the screen is shown. A wildcard that is
never set keeps whatever placeholder the text database holds, and a design-time placeholder
that ships to users looks exactly like real data.

Buttons: **R1** saves the current position as the target, **R2** exits.

## Message flow

The service owns the configuration; the GUI only asks and displays.

```
   GPS fix                                                    R1 pressed
      |                                                            |
      v                                                            v
   Service                                                        GUI
   distance + bearing                              SAVE_TARGET_HERE ->
      |                                                            |
      | NAV_UPDATE (NavState: name, distance, bearing, flags)      |
      +----------------------------------------------------------> |
                                                                   |
      <- setFloat + save() + re-read, then TARGET_SAVED ---------- +
```

`NavState` is a plain struct rather than message fields, because a `MessageBase` is
pool-allocated and non-copyable - the GUI could not keep a copy of the last update to redraw
from.

## Next Steps

- [App Configuration Fields](../../app-config-fields.md) - the full contract, including the
  companion-app specification and every limit
- [Files](../Files/ARCHITECTURE.md) - settings the user edits on the watch, which is the
  other half of this story
- [Sensors](tutorials/sensors/architecture) - the rest of the sensor layer this app only dips
  into

## Troubleshooting

**Every value is the default.** The app could not use the file. The log names the reason for
a file that exists but cannot be used — "not valid JSON", "has no schema", "over the
8192-byte limit" — and a `schema` other than `1` lands here by design. A file that simply
isn't there is not logged, so check `isLoaded()` if you need to tell "missing" from
"unusable".

**One value is the default and the others are fine.** That key is the wrong JSON type for its
declared field, or `null`, or a number with a fractional part in an `int` field. The companion
app cannot produce any of those; a hand edit can.

**The value is not what the phone shows.** It was outside `min`..`max` and got clamped, or the
string was longer than `maxLength` and got truncated. Neither is logged — they happen silently
inside the getters — so compare the file against your field table if a value looks capped.
CI keeps the two in step, but a hand-edited file can hold anything.

**A change on the phone did nothing.** Values are read at launch. Close the app and reopen it.

**A confident distance appears before there is a fix.** `GpsLocation::isDataValid()` only
says the sample is a well-formed GPS record; the fix itself is `isCoordinatesValid()`. Check
both, or a fix-less sample reads as latitude 0, longitude 0 and you will navigate to the Gulf
of Guinea.

**The watch keeps alerting after you leave the app.** The kernel does not stop a service when
its GUI closes, so a service that does not end itself keeps running - holding its sensor
subscriptions open and still firing alerts. Waypoint returns from `run()` on
`COMMAND_APP_NOTIF_GUI_STOP`; see the [Stopwatch](../../Examples/Stopwatch-Architecture.md)
notes for the case where staying alive is the right choice.

**CI fails with "field table ... but config.json".** The two declarations disagree. The error
names the field and both values; fix whichever is wrong.

**"config.json declares configFile X, but the checked sources only name Y".** The app opens a
different filename than it declared. Update `kFileName` or `configFile` so they match.
