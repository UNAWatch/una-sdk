<#
.SYNOPSIS
    Update Una-Watch apps over the USB (mass-storage) cable, preserving each app's
    settings and activity data.

.DESCRIPTION
    Replaces the .uapp binary inside each on-device app folder (<Drive>:\Apps\<App>\)
    with a newer build, WITHOUT touching that app's settings.json or Activity\ data.
    Avoids the two painful options: (a) copy new .uapp + delete old, per app, or
    (b) bulk-delete the app folders, which also wipes settings and activities.

    Source layout: the una-apps release (a .zip, or its extracted folder) is a set of
    per-app SUBFOLDERS whose names match the on-device folders, each holding one
    .uapp -- e.g. GlanceHR\Live_HR_1.3.0.uapp. The app is keyed by the SUBFOLDER name
    (the .uapp's own filename differs, especially for the Glance apps), so this maps
    <Source>\<App>\*.uapp  ->  <Drive>:\Apps\<App>\.

    Portable across machines: the watch is found by its VOLUME LABEL ("UNA WATCH"),
    not a hard-coded drive letter. Works on Windows PowerShell 5.1+ and PowerShell 7.

    The watch scans <Drive>:\Apps\<App>\ on boot and uses the FIRST .uapp it finds in
    each folder, so this removes any stale .uapp and leaves exactly one.

    Device notes handled / warned about:
      * Copy uses [System.IO.File]::Copy, NOT Copy-Item (scripted Copy-Item to this
        removable volume has produced silent bit-level corruption).
      * The new .uapp is copied BEFORE the old is removed.
      * A hash right after copying reads the Windows write cache and can be a false
        OK; -Verify is only trustworthy after eject (use -Eject, which flushes it).
      * The launcher / app list rebuilds only at boot -> reboot the watch after.
      * A .uapp that fails the kernel CRC is silently dropped (app won't appear).
      * D:\Apps also contains "SharedData" (not an app) - never touched, since there
        is no matching source subfolder. HRMonitor ships in the zip but not on the
        watch - skipped under the default (update-only).

.PARAMETER Source
    The una-apps release: either the .zip file or an already-extracted folder
    containing the per-app subfolders.

.PARAMETER Drive
    The watch's USB drive, e.g. E: - overrides label auto-detection.

.PARAMETER Label
    Volume label to match when auto-detecting the watch. Default: "UNA WATCH".

.PARAMETER InstallNew
    Also install apps present in -Source but not yet on the watch. Default: only
    UPDATE apps already on the watch.

.PARAMETER Verify
    Compare SHA-256 of each on-device .uapp against the source; run AFTER
    eject+reconnect (or -Eject+reconnect) so it reads cold flash, not the cache.

.PARAMETER Eject
    After copying, safely eject (flush + remove) the volume. Reconnect, then -Verify.

.EXAMPLE
    .\Update-Watch-Apps.ps1 -Source .\una-apps-apps-v1.3.0-rc3.zip -WhatIf
.EXAMPLE
    .\Update-Watch-Apps.ps1 -Source .\una-apps-apps-v1.3.0-rc3.zip -Eject
    # reconnect the watch, then:
    .\Update-Watch-Apps.ps1 -Source .\una-apps-apps-v1.3.0-rc3.zip -Verify
#>
[CmdletBinding(SupportsShouldProcess, ConfirmImpact = 'Medium')]
param(
    [Parameter(Mandatory)][string]$Source,
    [string]$Drive,
    [string]$Label = 'UNA WATCH',
    [switch]$InstallNew,
    [switch]$Verify,
    [switch]$Eject
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

# ---- resolve source (folder, or a .zip we auto-extract) --------------------
$origSource = $Source     # what to show the user for the follow-up -Verify command
$tmp = $null              # temp extract dir (cleaned up at the end) if -Source is a .zip
if (Test-Path -LiteralPath $Source -PathType Leaf) {
    if ($Source -notmatch '\.zip$') { throw "Source must be a folder or a .zip: $Source" }
    $tmp = Join-Path ([IO.Path]::GetTempPath()) ('uapp-src-' + [Guid]::NewGuid().ToString('N'))
    Expand-Archive -LiteralPath $Source -DestinationPath $tmp -Force
    $Source = $tmp
} elseif (-not (Test-Path -LiteralPath $Source -PathType Container)) {
    throw "Source not found: $Source"
}

# Each app is a SUBFOLDER (name == on-device folder) holding one .uapp.
$srcApps = @()
foreach ($d in Get-ChildItem -LiteralPath $Source -Directory) {
    $u = @(Get-ChildItem -LiteralPath $d.FullName -Filter *.uapp -File)
    if ($u.Count -eq 0) { continue }
    if ($u.Count -gt 1) { Write-Warning "$($d.Name): $($u.Count) .uapp files in source subfolder - skipping (ambiguous which to install)"; continue }
    $srcApps += [pscustomobject]@{ App = $d.Name; File = $u[0] }
}
if ($srcApps.Count -eq 0) { throw "No <App>\*.uapp subfolders under $Source. Point -Source at the una-apps zip or its extracted folder." }

# ---- resolve the watch drive (label -> -Drive -> \Apps fallback) -----------
function Resolve-WatchDrive {
    param([string]$Drive, [string]$Label)
    if ($Drive) {
        $r = ($Drive.TrimEnd('\', ':') + ':\')
        if (-not (Test-Path -LiteralPath (Join-Path $r 'Apps'))) { Write-Warning "No \Apps folder on $r - is that the watch drive?" }
        return $r
    }
    $vols = @(Get-Volume -ErrorAction SilentlyContinue | Where-Object { $_.FileSystemLabel -eq $Label -and $_.DriveLetter })
    if ($vols.Count -eq 1) { return ("{0}:\" -f $vols[0].DriveLetter) }
    if ($vols.Count -gt 1) { throw ("Multiple volumes labelled '$Label': {0}. Pass -Drive." -f (($vols.DriveLetter) -join ', ')) }
    # fallback: a REMOVABLE drive with an \Apps folder (never a fixed drive like C:\Apps)
    $cands = @(Get-Volume -ErrorAction SilentlyContinue |
        Where-Object { $_.DriveType -eq 'Removable' -and $_.DriveLetter -and (Test-Path -LiteralPath ("{0}:\Apps" -f $_.DriveLetter)) } |
        ForEach-Object { "{0}:\" -f $_.DriveLetter })
    if ($cands.Count -eq 1) { Write-Warning "No volume labelled '$Label'; using $($cands[0]) (removable, has an \Apps folder)."; return $cands[0] }
    throw "Could not find the watch (no volume labelled '$Label', no single removable drive with \Apps). Connect it, or pass -Drive."
}
$root     = Resolve-WatchDrive -Drive $Drive -Label $Label
$appsRoot = Join-Path $root 'Apps'
if (-not (Test-Path -LiteralPath $appsRoot)) { throw "No Apps folder at $appsRoot." }
Write-Host "Watch apps folder: $appsRoot  ($($srcApps.Count) apps in source)"

# ---- locale-independent safe eject (P/Invoke storage IOCTLs) --------------
function Invoke-SafeEject([string]$root) {
    $letter = ($root.TrimEnd('\')).TrimEnd(':')
    if (-not ([System.Management.Automation.PSTypeName]'Una.VolumeEject').Type) {
        # -TypeDefinition (full namespace/class) so the `using` directives are legal.
        Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
namespace Una {
    public static class VolumeEject {
        const uint GENERIC_RW = 0x80000000 | 0x40000000, SHARE_RW = 0x1 | 0x2, OPEN_EXISTING = 3;
        const uint FSCTL_LOCK_VOLUME = 0x00090018, FSCTL_DISMOUNT_VOLUME = 0x00090020;
        const uint IOCTL_MEDIA_REMOVAL = 0x002D4804, IOCTL_EJECT_MEDIA = 0x002D4808;
        [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern SafeFileHandle CreateFile(string n, uint a, uint s, IntPtr sec, uint d, uint f, IntPtr t);
        [DllImport("kernel32", SetLastError = true)]
        static extern bool DeviceIoControl(SafeFileHandle h, uint code, byte[] ib, uint ibl, byte[] ob, uint obl, out uint br, IntPtr ov);
        public static string Eject(char letter) {
            var h = CreateFile(@"\\.\" + letter + ":", GENERIC_RW, SHARE_RW, IntPtr.Zero, OPEN_EXISTING, 0, IntPtr.Zero);
            if (h.IsInvalid) return "open failed (err " + Marshal.GetLastWin32Error() + ")";
            try {
                uint br; bool locked = false;
                for (int i = 0; i < 12 && !locked; i++) { locked = DeviceIoControl(h, FSCTL_LOCK_VOLUME, null, 0, null, 0, out br, IntPtr.Zero); if (!locked) System.Threading.Thread.Sleep(250); }
                if (!locked) return "lock failed - files still open on the volume";
                if (!DeviceIoControl(h, FSCTL_DISMOUNT_VOLUME, null, 0, null, 0, out br, IntPtr.Zero)) return "dismount failed (err " + Marshal.GetLastWin32Error() + ")";
                DeviceIoControl(h, IOCTL_MEDIA_REMOVAL, new byte[] { 0 }, 1, null, 0, out br, IntPtr.Zero);
                if (!DeviceIoControl(h, IOCTL_EJECT_MEDIA, null, 0, null, 0, out br, IntPtr.Zero)) return "eject failed (err " + Marshal.GetLastWin32Error() + ")";
                return "OK";
            } finally { h.Close(); }
        }
    }
}
'@
    }
    $res = [Una.VolumeEject]::Eject([char]$letter)
    if ($res -ne 'OK') { Write-Warning "Auto-eject failed: $res. Eject $letter`: manually via the tray."; return $false }
    for ($i = 0; $i -lt 20; $i++) { Start-Sleep -Milliseconds 500; if (-not (Test-Path -LiteralPath $root)) { return $true } }
    Write-Warning "$letter`: still present after eject. Eject it manually via the tray."; return $false
}

# ===========================================================================
# VERIFY mode
# ===========================================================================
if ($Verify) {
    Write-Host "`nVerify - ensure you EJECTED and RECONNECTED the watch first, else this reads"
    Write-Host "the Windows write cache and can report a false OK.`n"
    $bad = 0
    foreach ($a in $srcApps) {
        $dst = Join-Path (Join-Path $appsRoot $a.App) $a.File.Name
        if (-not (Test-Path -LiteralPath $dst)) { Write-Host ("  [MISSING ] {0}\{1}" -f $a.App, $a.File.Name); $bad++; continue }
        if ((Get-FileHash -LiteralPath $a.File.FullName -Algorithm SHA256).Hash -eq (Get-FileHash -LiteralPath $dst -Algorithm SHA256).Hash) {
            Write-Host ("  [OK      ] {0}\{1}" -f $a.App, $a.File.Name)
        } else { Write-Host ("  [MISMATCH] {0}\{1}" -f $a.App, $a.File.Name); $bad++ }
    }
    if ($tmp) { Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue -WhatIf:$false -Confirm:$false }
    if ($bad -gt 0) { Write-Warning "$bad file(s) failed - re-copy, then eject/reconnect and verify again."; exit 1 }
    Write-Host "`nAll files verified against flash. Reboot the watch to refresh the launcher."
    return
}

# ===========================================================================
# UPDATE / INSTALL
# ===========================================================================
$updated = @(); $installed = @(); $skipped = @(); $failed = @()
foreach ($a in $srcApps) {
    $appDir = Join-Path $appsRoot $a.App
    $exists = Test-Path -LiteralPath $appDir -PathType Container

    if (-not $exists -and -not $InstallNew) { $skipped += $a.App; Write-Host ("  [skip    ] {0} (not on watch; -InstallNew to add)" -f $a.App); continue }

    $dst    = Join-Path $appDir $a.File.Name
    $action = if ($exists) { 'update' } else { 'install' }
    if ($PSCmdlet.ShouldProcess("$($a.App)  ->  $dst", $action)) {
        if (-not $exists) { New-Item -ItemType Directory -Path $appDir -Force | Out-Null }
        [System.IO.File]::Copy($a.File.FullName, $dst, $true)                  # new first, via .NET (not Copy-Item)
        $dstLen = (Get-Item -LiteralPath $dst).Length
        if ($a.File.Length -ne $dstLen) {
            # bad copy: remove the corrupt new file and KEEP the old .uapp so the app still works
            Write-Warning ("  {0}: size mismatch (src={1} dev={2}) - bad copy; removed it, kept the existing .uapp" -f $a.App, $a.File.Length, $dstLen)
            [System.IO.File]::Delete($dst)
            $failed += $a.App
            continue
        }
        # copy validated -> now safe to remove stale .uapp(s); keep settings.json / Activity
        Get-ChildItem -LiteralPath $appDir -Filter *.uapp -File | Where-Object { $_.Name -ne $a.File.Name } | ForEach-Object {
            [System.IO.File]::Delete($_.FullName); Write-Host ("    removed old {0}" -f $_.Name)
        }
        if ($exists) { $updated += $a.App } else { $installed += $a.App }
        Write-Host ("  [{0,-8}] {1} -> {2}" -f $action, $a.App, $a.File.Name)
    }
}

Write-Host ""
Write-Host ("Updated ({0}):   {1}" -f $updated.Count,   $(if ($updated)   { $updated   -join ', ' } else { '-' }))
Write-Host ("Installed ({0}): {1}" -f $installed.Count, $(if ($installed) { $installed -join ', ' } else { '-' }))
Write-Host ("Skipped ({0}):   {1}" -f $skipped.Count,   $(if ($skipped)   { $skipped   -join ', ' } else { '-' }))
if ($failed.Count) { Write-Warning ("Failed ({0}):    {1}  (old .uapp left in place)" -f $failed.Count, ($failed -join ', ')) }
Write-Host ""

if ($Eject -and $PSCmdlet.ShouldProcess($root, 'safely eject')) {
    Write-Host "Ejecting $root ..."
    if (Invoke-SafeEject $root) {
        Write-Host "Ejected. Reconnect the watch, then:"
        Write-Host ("  .\Update-Watch-Apps.ps1 -Source '{0}' -Verify" -f $origSource)
        Write-Host "and reboot the watch to rebuild the launcher."
    }
} else {
    Write-Host "NEXT STEPS:"
    Write-Host "  1. Safely eject the watch (tray -> Safely Remove Hardware), then reconnect. (Or re-run with -Eject.)"
    Write-Host ("  2. Verify flash:  .\Update-Watch-Apps.ps1 -Source '{0}' -Verify" -f $origSource)
    Write-Host "  3. Reboot the watch so it rebuilds the launcher / app list."
}

if ($tmp) { Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue }
