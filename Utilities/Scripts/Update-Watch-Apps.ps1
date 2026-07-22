<#
.SYNOPSIS
    Update Una-Watch apps over the USB (mass-storage) cable, preserving each app's
    settings and activity data.

.DESCRIPTION
    Replaces the .uapp binary inside each on-device app folder (<Drive>:\Apps\<App>\)
    with a newer build, WITHOUT touching that app's settings.json or Activity\ data.
    Avoids the two painful options: (a) copy new .uapp + delete old, per app, or
    (b) bulk-delete the app folders, which also wipes settings and activities.

    Portable across machines: the watch is found by its VOLUME LABEL ("UNA WATCH"),
    not a hard-coded drive letter, so it works whatever letter it mounts as and on
    anyone's PC. Works on Windows PowerShell 5.1+ (built in) and PowerShell 7.

    The watch scans <Drive>:\Apps\<App>\ on boot and uses the FIRST .uapp it finds
    in each folder, so this removes any stale .uapp and leaves exactly one.

    Device notes handled / warned about:
      * Copy uses [System.IO.File]::Copy, NOT Copy-Item (scripted Copy-Item to this
        removable volume has produced silent bit-level corruption).
      * The new .uapp is copied BEFORE the old is removed (a failed copy never
        leaves you with nothing).
      * A hash taken right after copying reads the Windows write cache and can be a
        false OK; -Verify is only trustworthy after the volume is ejected (or use
        -Eject, which flushes + removes it).
      * The launcher / app list rebuilds only at boot -> reboot the watch after.
      * A .uapp that fails the kernel CRC is silently dropped (app just won't appear).
      * D:\Apps also contains "SharedData" (not an app); update-only-by-source means
        it is never touched.

.PARAMETER Source
    Folder containing the new <App>_<version>.uapp files (e.g. an extracted
    una-apps-<tag>.zip).

.PARAMETER Drive
    The watch's USB drive, e.g. E: - overrides label auto-detection.

.PARAMETER Label
    Volume label to match when auto-detecting the watch. Default: "UNA WATCH".

.PARAMETER InstallNew
    Also install apps present in -Source but not yet on the watch. Default: only
    UPDATE apps already on the watch.

.PARAMETER Verify
    Compare SHA-256 of each on-device .uapp against the source and report
    mismatches. Run AFTER ejecting + reconnecting the watch (or after -Eject +
    reconnect) so it reads cold flash, not the write cache.

.PARAMETER Eject
    After copying, safely eject (flush + remove) the volume. Reconnect the watch,
    then re-run with -Verify.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\Update-Watch-Apps.ps1 -Source .\una-apps-apps-v1.3.0-rc3 -WhatIf
.EXAMPLE
    .\Update-Watch-Apps.ps1 -Source .\una-apps-apps-v1.3.0-rc3 -Eject
    # reconnect the watch, then:
    .\Update-Watch-Apps.ps1 -Source .\una-apps-apps-v1.3.0-rc3 -Verify
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

# ---- source files ----------------------------------------------------------
if (-not (Test-Path -LiteralPath $Source -PathType Container)) { throw "Source folder not found: $Source" }
$srcFiles = @(Get-ChildItem -LiteralPath $Source -Filter *.uapp -File)
if ($srcFiles.Count -eq 0) { throw "No .uapp files found in $Source (point -Source at the folder holding them)." }

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
    # fallback: a drive that has an \Apps folder
    $cands = @(Get-PSDrive -PSProvider FileSystem | Where-Object { $_.Root -match '^[A-Za-z]:\\$' -and (Test-Path -LiteralPath (Join-Path $_.Root 'Apps')) })
    if ($cands.Count -eq 1) { Write-Warning "No volume labelled '$Label'; using $($cands[0].Root) (it has an \Apps folder)."; return $cands[0].Root }
    throw "Could not find the watch (no volume labelled '$Label', no unambiguous \Apps drive). Connect it, or pass -Drive."
}
$root     = Resolve-WatchDrive -Drive $Drive -Label $Label
$appsRoot = Join-Path $root 'Apps'
if (-not (Test-Path -LiteralPath $appsRoot)) { throw "No Apps folder at $appsRoot." }
Write-Host "Watch apps folder: $appsRoot"

# ---- <App>_<version>.uapp  ->  <App> --------------------------------------
function Get-AppName([string]$fileName) {
    if ($fileName -match '^(?<name>.+?)_\d[\w.\-]*\.uapp$') { return $Matches['name'] }
    return [IO.Path]::GetFileNameWithoutExtension($fileName)
}

# ---- locale-independent safe eject (P/Invoke storage IOCTLs) --------------
function Invoke-SafeEject([string]$root) {
    $letter = ($root.TrimEnd('\')).TrimEnd(':')
    if (-not ([System.Management.Automation.PSTypeName]'Una.VolumeEject').Type) {
        Add-Type -Namespace Una -Name VolumeEject -Language CSharp -MemberDefinition @'
    using System;
    using System.Runtime.InteropServices;
    using Microsoft.Win32.SafeHandles;
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
            DeviceIoControl(h, IOCTL_MEDIA_REMOVAL, new byte[] { 0 }, 1, null, 0, out br, IntPtr.Zero); // allow removal
            if (!DeviceIoControl(h, IOCTL_EJECT_MEDIA, null, 0, null, 0, out br, IntPtr.Zero)) return "eject failed (err " + Marshal.GetLastWin32Error() + ")";
            return "OK";
        } finally { h.Close(); }
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
    foreach ($f in $srcFiles) {
        $app = Get-AppName $f.Name
        $dst = Join-Path (Join-Path $appsRoot $app) $f.Name
        if (-not (Test-Path -LiteralPath $dst)) { Write-Host ("  [MISSING ] {0}\{1}" -f $app, $f.Name); $bad++; continue }
        if ((Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash -eq (Get-FileHash -LiteralPath $dst -Algorithm SHA256).Hash) {
            Write-Host ("  [OK      ] {0}\{1}" -f $app, $f.Name)
        } else { Write-Host ("  [MISMATCH] {0}\{1}" -f $app, $f.Name); $bad++ }
    }
    if ($bad -gt 0) { Write-Warning "$bad file(s) failed - re-copy, then eject/reconnect and verify again."; exit 1 }
    Write-Host "`nAll files verified against flash. Reboot the watch to refresh the launcher."
    return
}

# ===========================================================================
# UPDATE / INSTALL
# ===========================================================================
$updated = @(); $installed = @(); $skipped = @()
foreach ($f in $srcFiles) {
    $app    = Get-AppName $f.Name
    $appDir = Join-Path $appsRoot $app
    $exists = Test-Path -LiteralPath $appDir -PathType Container

    if (-not $exists -and -not $InstallNew) { $skipped += $app; Write-Host ("  [skip    ] {0} (not on watch; -InstallNew to add)" -f $app); continue }

    $dst    = Join-Path $appDir $f.Name
    $action = if ($exists) { 'update' } else { 'install' }
    if ($PSCmdlet.ShouldProcess("$app  ->  $dst", $action)) {
        if (-not $exists) { New-Item -ItemType Directory -Path $appDir -Force | Out-Null }
        [System.IO.File]::Copy($f.FullName, $dst, $true)                       # new first, via .NET (not Copy-Item)
        $dstLen = (Get-Item -LiteralPath $dst).Length
        if ($f.Length -ne $dstLen) { Write-Warning ("  size mismatch on {0}: src={1} dev={2} - copy may be bad" -f $app, $f.Length, $dstLen) }
        Get-ChildItem -LiteralPath $appDir -Filter *.uapp -File | Where-Object { $_.Name -ne $f.Name } | ForEach-Object {
            [System.IO.File]::Delete($_.FullName); Write-Host ("    removed old {0}" -f $_.Name)   # stale .uapp only; keep settings/Activity
        }
        if ($exists) { $updated += $app } else { $installed += $app }
        Write-Host ("  [{0,-8}] {1} -> {2}" -f $action, $app, $f.Name)
    }
}

Write-Host ""
Write-Host ("Updated ({0}):   {1}" -f $updated.Count,   $(if ($updated)   { $updated   -join ', ' } else { '-' }))
Write-Host ("Installed ({0}): {1}" -f $installed.Count, $(if ($installed) { $installed -join ', ' } else { '-' }))
Write-Host ("Skipped ({0}):   {1}" -f $skipped.Count,   $(if ($skipped)   { $skipped   -join ', ' } else { '-' }))
Write-Host ""

if ($Eject -and -not $WhatIfPreference) {
    Write-Host "Ejecting $root ..."
    if (Invoke-SafeEject $root) {
        Write-Host "Ejected. Reconnect the watch, then:"
        Write-Host ("  .\Update-Watch-Apps.ps1 -Source '{0}' -Verify" -f $Source)
        Write-Host "and reboot the watch to rebuild the launcher."
    }
} else {
    Write-Host "NEXT STEPS:"
    Write-Host "  1. Safely eject the watch (tray -> Safely Remove Hardware), then reconnect. (Or re-run with -Eject.)"
    Write-Host ("  2. Verify flash:  .\Update-Watch-Apps.ps1 -Source '{0}' -Verify" -f $Source)
    Write-Host "  3. Reboot the watch so it rebuilds the launcher / app list."
}
