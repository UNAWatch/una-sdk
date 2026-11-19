---
name: una-store-submit
description: Prepare a UNA Watch app release for the UNA app store (apps.unawatch.com) - store APP_ID, app-manifest.json with minKernelVersion stamped, a clean tagged build, validation and the upload zip - for the developer to upload and publish. Also covers contributing an open-source app to the SDK. Use when releasing, publishing or submitting a UNA Watch app.
---

# Prepare a UNA Watch app for the store

You prepare everything. The developer signs in to the store, uploads and publishes. Never
upload or publish for them, and don't ask for their store credentials.

Closed-source apps go through the store portal, <https://apps.unawatch.com>. Open-source apps
can instead be contributed to the SDK (see the end).

## 1. Store APP_ID

A store app needs the ID the store issues, not a locally made one. The developer:
1. signs in at <https://apps.unawatch.com>;
2. clicks **Add New**;
3. enters the app name and a short description, then clicks **Generate**;
4. copies the **App ID** from the app's page.

Then you:
- set it as `APP_ID` in the app's `CMakeLists.txt`;
- set the same ID in the simulator's CMake definitions, if the app has a CMake simulator;
- reconfigure (`cmake -S ... -B build`, not just a rebuild).

The phone app matches new versions of an app to the installed one by this ID, so it must never
change between versions.

## 2. Agree the version

Confirm the release version with the developer, for example `1.2.0`. The manifest and the tag
both carry it.

## 3. app-manifest.json

The manifest tells the phone app and the store about the app. It isn't installed on the watch.
Keep it at the app's root as `app-manifest.json`, committed with the app. (The examples'
`Output/` folder ignores everything but `Output/Release/`.) It must be strict JSON, with no
comments.

```json
{
  "manifest_version": 1,
  "type": ["activity"],
  "name": "My App",
  "icon": "icon.png",
  "binary": "MyApp_1.2.0.uapp",
  "appVersion": "1.2.0",
  "minKernelVersion": "1.4.0",
  "requiredHardware": ["GPS", "HR"],
  "description": "One or two sentences for the store listing.",
  "id": "03AD5A741E38A35F",
  "supportsLaps": false,
  "supportsDistance": false,
  "supportsTrack": false,
  "supportsHeartbeat": false,
  "supportsElevation": false,
  "supportsStep": false,
  "supportsSpeed": "none",
  "customMeasures": [],
  "stravaExport": false
}
```

| Key | Value |
|---|---|
| `manifest_version` | Always `1`. |
| `type` | One or more of `activity`, `utility`, `glance`, `clockface`, matching `APP_TYPE`. |
| `name`, `description` | As shown in the store and phone app. |
| `icon` | The icon's path inside the zip. |
| `binary` | The `.uapp` file name inside the zip, exactly as the release build will name it (skill `una-build-uapp`). |
| `appVersion` | The agreed version (`1.2.0`). |
| `minKernelVersion` | Set by the SDK's tool (below), never by hand below its floor. |
| `requiredHardware` | Hardware the app can't work without: `GPS`, `ACCELEROMETER`, `HR`, and so on. |
| `id` | The store `APP_ID`. |
| `supports*`, `customMeasures`, `stravaExport` | Activity apps: what the phone app shows for a recorded activity. Use `false`, `"none"` and `[]` for other types. |
| `configFile`, `configFields` | Only if the app asks the phone for settings. |

The full list of keys, including custom metrics and config fields, is in the SDK's
`Docs/app-config-json.md` and `Docs/app-config-fields.md`.

### minKernelVersion

The app needs a watch firmware that provides the SDK interface (ABI) it was built against. The
SDK maps that ABI to the lowest firmware version. Stamp it from the SDK the app is built with:

```bash
python3 "$UNA_SDK/Utilities/Scripts/app_packer/min_kernel_version.py" --stamp app-manifest.json
python3 "$UNA_SDK/Utilities/Scripts/app_packer/min_kernel_version.py" --check app-manifest.json
```

`--stamp` raises the value to the floor and keeps a higher one. A higher value is allowed when the
app needs a specific firmware fix; a lower one is not.

### Validate config fields

If the manifest has `configFields`:

```bash
python3 "$UNA_SDK/Utilities/Scripts/app_packer/validate_app_config.py" --check app-manifest.json \
    --check-bounds Software/Libs/Sources/<file holding the SDK::AppConfig::Field table>.cpp
```

## 4. Commit, tag and build

The release build must come from a committed, tagged state:

1. Commit the manifest and any last changes.
2. `git status` is clean: no uncommitted changes. Check it yourself: a tagged commit builds
   with the plain version even when the working tree has changes, so the version alone doesn't
   prove the build is clean.
3. Tag the release commit `apps-v<major>.<minor>.<patch>`, for example `apps-v1.2.0`. Pushing
   the tag is the developer's call.
4. Build from scratch: delete `build/`, configure, build (skill `una-build-uapp`).
5. Check the build log:
   - the packer's `ID` line matches the store `APP_ID`;
   - the `.uapp` name carries the plain version (`MyApp_1.2.0.uapp`), with no commit count,
     hash or `-dirty`, and matches the manifest's `binary`.
6. Run the unit tests (skill `una-unit-test`) and a scripted simulator run (skill `una-simulator`)
   on that build.
7. Have the developer test that exact `.uapp` on a watch (skill `una-test-on-watch`).

If anything has to change after tagging, fix it in a new commit and tag a new version.

## 5. The upload zip

A zip holding, at its root:
- the `.uapp`;
- `app-manifest.json`;
- the icon named by `icon` (a PNG; the app's `Resources/icon_60x60.png` is the usual source).

Optionally, add preview screenshots in a `previews/` folder named by a `"previews"` key. Scripted
simulator screenshots work well for this (skill `una-simulator`).

Start from an empty `release/` folder and no old zip, so nothing from an earlier release goes
in:

```bash
rm -rf release MyApp_1.2.0.zip
mkdir release && cp Output/MyApp_1.2.0.uapp app-manifest.json release/
cp Resources/icon_60x60.png release/icon.png
(cd release && zip -r ../MyApp_1.2.0.zip .)
```

```powershell
Remove-Item -Recurse -Force release, MyApp_1.2.0.zip -ErrorAction SilentlyContinue
New-Item -ItemType Directory release | Out-Null
Copy-Item Output\MyApp_1.2.0.uapp, app-manifest.json release\
Copy-Item Resources\icon_60x60.png release\icon.png
Compress-Archive -Path release\* -DestinationPath MyApp_1.2.0.zip
```

Before handing it over, check:
- `binary` and `icon` name files that are in the zip;
- `id` equals the build's ID;
- `appVersion` equals the version in the `.uapp`'s file name (`1.2.0` in `MyApp_1.2.0.uapp`).

## 6. Hand over

Give the developer:
- the zip's path;
- the version;
- the `minKernelVersion`;
- what was tested, and where.

Then the steps they take on the store:
1. Open the app's page and go to the **Version** tab.
2. Click **Upload New** and choose the zip.
3. After the upload succeeds, click **Release**, then **Confirm and Publish**.

## Open-source apps: contributing to the SDK

An app can instead be offered to the SDK's examples by pull request to
<https://github.com/UNAWatch/una-sdk>, under `Examples/Apps/<AppName>/`, laid out like the other
examples. Example apps are built and shipped by the SDK's own release process, so the pull request
contains source only: no `.uapp` and no zip. Opening the pull request is the developer's decision.
