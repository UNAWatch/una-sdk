---
name: una-test-on-watch
description: Install a UNA Watch app (.uapp) on a real watch over USB, collect the watch's log files, and run through the checks only a watch can answer. Use when installing or updating an app on a UNA Watch, when an installed app doesn't appear or closes, or when reading logs or crash dumps from the watch.
---

# Test a UNA Watch app on the watch

The user has to do the physical steps: plug in, eject, restart, press buttons, wear it. You
prepare the files, tell them exactly what to do, verify what you can and read the logs. Say
clearly which steps are theirs.

## Install

1. **Build** the `.uapp` (skill `una-build-uapp`).
2. **Connect.** The user connects the watch by USB. It mounts as a USB drive (volume label
   `UNA WATCH` on most watches). Running apps may flush their data first, so it can take a few
   seconds. Find the drive by its label, or ask the user which drive it is. Don't assume a drive
   letter or mount point.
3. **Copy into the app's folder.** The `.uapp` goes in `Apps/<AppName>/` on the drive. Create the
   folder for a new app. Folder name and file name are free, but use the app's name.
   - The watch loads the first `.uapp` it finds in each folder, so a folder must hold exactly one.
     When updating, copy the new `.uapp` in first. If it has the same file name as the old one
     (untagged builds are all `<Name>_0.0.0-dev.uapp`), the copy overwrites it and nothing is
     removed. Otherwise, then remove the other `.uapp` files in the folder, never the one just
     copied.
   - On Windows, copy with .NET rather than `Copy-Item`, with absolute paths (.NET doesn't use
     PowerShell's current folder): `[System.IO.File]::Copy($src, $dst, $true)`, where `$true`
     allows overwriting. Verify by hash afterwards (below).
   - Leave everything else in the folder alone: `settings.json`, `Activity/` and similar are the
     user's data.
   - `Apps/SharedData/` isn't an app. Never touch it.
   - Copying is the user's call. Ask before writing to their watch. They may prefer to copy it
     themselves in Explorer or Finder.
4. **Eject safely, then disconnect.** Eject from the OS (Explorer, Finder, `udisksctl
   power-off`, …) before unplugging, or the copy may not be complete on the watch.
5. **Restart the watch.** The app list is rebuilt only when the watch starts, so a new or updated
   app appears only after it is turned off and on again.
6. **Open the app.** The top-right button opens the app list.

On Windows, `Utilities/Scripts/Update-Watch-Apps.ps1` in the SDK updates apps already on the
watch, with `-InstallNew` for new ones. It finds the watch by its label, keeps settings and
activities, copies the new `.uapp` before removing the old one, and has `-Eject` and `-Verify`.
`-Source` is a folder (or `.zip`) of per-app subfolders, each holding one `.uapp`. The subfolder
name picks the folder on the watch, so it must match the app's folder under `Apps/`: a build's
`Output/` folder isn't in that layout. For example, put the `.uapp` in `staging\MyApp\` and
pass `-Source staging`.

## Verify the copy

The watch checks each `.uapp` when it starts and silently leaves out one that is damaged: the app
just doesn't appear. A damaged copy can still have the right size, and a hash right after copying
can read the computer's cache rather than the watch. So, to verify:

1. Eject, then reconnect the watch.
2. Hash the file on the watch and compare it with the build:

```bash
sha256sum Output/MyApp_1.2.0.uapp "/media/$USER/UNA WATCH/Apps/MyApp/MyApp_1.2.0.uapp"
```

```powershell
Get-FileHash Output\MyApp_1.2.0.uapp, E:\Apps\MyApp\MyApp_1.2.0.uapp -Algorithm SHA256
```

If the hashes differ, delete the file on the watch, copy it again and verify again.

## Logs

The watch can write its system log, including the app's `LOG_INFO`/`LOG_WARNING`/`LOG_ERROR`
lines, to files on its drive:

1. **Turn logging on.** Write `debug_settings.json` at the root of the watch's drive:
   ```json
   { "logCfg": { "logToFile": true } }
   ```
   If the file exists, note its current `logCfg.logToFile` value, then set only that and keep
   its other settings.
2. **Eject, disconnect and restart the watch** (the setting is read at start-up). Then reproduce
   the problem.
3. **Collect the files.** Reconnect, and read `Log/System/syslog_<id>_<NNNN>_<timestamp>.log`.
   - Files roll over at 1 MB and the newest 10 sessions are kept. The highest `NNNN` is the
     latest.
   - Lines carry the app's module prefix: the `LOG_MODULE_PRX` set at the top of each source
     file.
4. **Put the setting back** when done: the value noted in step 1, or `false` if the file didn't
   exist or the setting wasn't there. Logging costs battery and flash wear. Then eject,
   disconnect and restart the watch.

**Crashes:** crash dumps land in `Crash/` on the drive. Include them when reporting a crash.

The same log is also sent out of the watch's debug UART, which needs a debug adapter.

## Checks only a watch can answer

Walk the user through these. Report each answer: they are the gaps the simulator can't cover.

- **It appears and opens:**
  - the app is in the list with the right name and icon;
  - it opens, every screen can be reached, and the back button (R2) leaves it.
- **Messages:** values sent from the service show on screen. If the GUI shows defaults or
  zeros where the simulator showed values, a message is probably over 256 bytes: the watch drops
  those silently.
- **Display:**
  - text and icons are readable on the panel, which shows 2 bits per colour channel;
  - nothing is cut off at the round edge.
- **Timing:**
  - screens keep up;
  - animations aren't too fast or slow;
  - buttons respond on the first press.
- **Sensors (outdoors where needed):**
  - real HR, GPS fix time and accuracy;
  - step counts;
  - wrist raise.
- **Lifecycle:**
  - leave the app and come back: a running activity keeps going and the screen restores;
  - an activity's files (for example FIT files) appear on the drive after saving.
- **Battery, for long-running apps:** the battery drop over a known time, compared with the watch
  without the app.

## Troubleshooting

- **App doesn't appear:**
  - the watch wasn't restarted;
  - the `.uapp` is damaged (verify the hash);
  - it isn't in its own folder under `Apps/`;
  - another `.uapp` in the same folder was loaded instead.
- **App appears but closes at once:** collect logs (above) and crash dumps from `Crash/`.
- **The phone app won't install a store app on this watch:** the watch's firmware may be older
  than the app's `minKernelVersion` (skill `una-store-submit`). The user updates the watch's
  firmware.
- **Problems that look like the platform:** report at <https://github.com/UNAWatch/una-sdk/issues>
  with the log files.
