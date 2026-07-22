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
    Add-Type -AssemblyName System.IO.Compression.FileSystem -ErrorAction SilentlyContinue   # PS 5.1 needs this; PS 7 has it built in
    # .NET extraction (creates $tmp) - no cmdlet ShouldProcess/prompt, so it works non-interactively and under -WhatIf
    [System.IO.Compression.ZipFile]::ExtractToDirectory((Resolve-Path -LiteralPath $Source).Path, $tmp)
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
    $vols = @(Get-Volume -ErrorAction SilentlyContinue | Where-Object { $_.FileSystemLabel -eq $Label -and $_.DriveLetter -and $_.DriveType -eq 'Removable' })
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

# ---- locale-independent safe eject (P/Invoke) ------------------------------
# The watch is a USB *composite* device, so IOCTL_STORAGE_EJECT_MEDIA on the
# volume only flushes -- it doesn't take the drive offline (the device just
# re-presents its media). A true "Safely Remove" is CM_Request_Device_Eject on
# the removable USB devnode: map the drive letter -> disk device number -> its
# devnode, then eject that node or the first ejectable ancestor up the tree.
function Invoke-SafeEject([string]$root) {
    $letter = ($root.TrimEnd('\')).TrimEnd(':')
    if (-not ([System.Management.Automation.PSTypeName]'Una.DeviceEject').Type) {
        # -TypeDefinition (full namespace/class) so the `using` directives are legal.
        Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
namespace Una {
    public static class DeviceEject {
        const uint SHARE_RW = 0x1 | 0x2, OPEN_EXISTING = 3, GENERIC_RW = 0x80000000 | 0x40000000;
        const uint IOCTL_STORAGE_GET_DEVICE_NUMBER = 0x2D1080;
        const uint FSCTL_LOCK_VOLUME = 0x00090018, FSCTL_DISMOUNT_VOLUME = 0x00090020;
        const int DIGCF_PRESENT = 0x2, DIGCF_DEVICEINTERFACE = 0x10;
        static Guid GUID_DISK = new Guid("53f56307-b6bf-11d0-94f2-00a0c91efb8b");

        [StructLayout(LayoutKind.Sequential)]
        struct STORAGE_DEVICE_NUMBER { public int DeviceType; public uint DeviceNumber; public uint PartitionNumber; }
        [StructLayout(LayoutKind.Sequential)]
        struct SP_DEVICE_INTERFACE_DATA { public int cbSize; public Guid guid; public int Flags; public IntPtr Reserved; }
        [StructLayout(LayoutKind.Sequential)]
        struct SP_DEVINFO_DATA { public int cbSize; public Guid ClassGuid; public uint DevInst; public IntPtr Reserved; }

        [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern SafeFileHandle CreateFile(string n, uint a, uint s, IntPtr sec, uint d, uint f, IntPtr t);
        [DllImport("kernel32", SetLastError = true)]
        static extern bool DeviceIoControl(SafeFileHandle h, uint code, IntPtr ib, uint ibl, IntPtr ob, uint obl, out uint br, IntPtr ov);
        [DllImport("setupapi", SetLastError = true)]
        static extern IntPtr SetupDiGetClassDevs(ref Guid g, IntPtr en, IntPtr hwnd, int flags);
        [DllImport("setupapi", SetLastError = true)]
        static extern bool SetupDiEnumDeviceInterfaces(IntPtr h, IntPtr di, ref Guid g, int idx, ref SP_DEVICE_INTERFACE_DATA d);
        [DllImport("setupapi", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern bool SetupDiGetDeviceInterfaceDetail(IntPtr h, ref SP_DEVICE_INTERFACE_DATA d, IntPtr det, int detSize, ref int req, ref SP_DEVINFO_DATA info);
        [DllImport("setupapi", SetLastError = true)]
        static extern bool SetupDiDestroyDeviceInfoList(IntPtr h);
        [DllImport("cfgmgr32")]
        static extern int CM_Get_Parent(out uint parent, uint dev, int flags);
        [DllImport("cfgmgr32", CharSet = CharSet.Unicode)]
        static extern int CM_Request_Device_EjectW(uint dev, out int veto, StringBuilder name, int len, int flags);

        static uint DeviceNumberOf(string path) {
            var h = CreateFile(path, 0, SHARE_RW, IntPtr.Zero, OPEN_EXISTING, 0, IntPtr.Zero);
            if (h.IsInvalid) return 0xFFFFFFFF;
            try {
                int sz = Marshal.SizeOf(typeof(STORAGE_DEVICE_NUMBER));
                IntPtr buf = Marshal.AllocHGlobal(sz);
                try {
                    uint br;
                    if (!DeviceIoControl(h, IOCTL_STORAGE_GET_DEVICE_NUMBER, IntPtr.Zero, 0, buf, (uint)sz, out br, IntPtr.Zero)) return 0xFFFFFFFF;
                    return ((STORAGE_DEVICE_NUMBER)Marshal.PtrToStructure(buf, typeof(STORAGE_DEVICE_NUMBER))).DeviceNumber;
                } finally { Marshal.FreeHGlobal(buf); }
            } finally { h.Close(); }
        }

        public static string Eject(char letter) {
            string vol = @"\\.\" + letter + ":";
            // flush + dismount first (also makes a later -Verify read cold flash)
            var vh = CreateFile(vol, GENERIC_RW, SHARE_RW, IntPtr.Zero, OPEN_EXISTING, 0, IntPtr.Zero);
            if (!vh.IsInvalid) {
                uint br; bool locked = false;
                for (int i = 0; i < 12 && !locked; i++) { locked = DeviceIoControl(vh, FSCTL_LOCK_VOLUME, IntPtr.Zero, 0, IntPtr.Zero, 0, out br, IntPtr.Zero); if (!locked) System.Threading.Thread.Sleep(250); }
                DeviceIoControl(vh, FSCTL_DISMOUNT_VOLUME, IntPtr.Zero, 0, IntPtr.Zero, 0, out br, IntPtr.Zero);
                vh.Close();
            }

            uint num = DeviceNumberOf(vol);
            if (num == 0xFFFFFFFF) return "could not read the volume's device number";

            IntPtr hDev = SetupDiGetClassDevs(ref GUID_DISK, IntPtr.Zero, IntPtr.Zero, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
            if (hDev == new IntPtr(-1)) return "SetupDiGetClassDevs failed";
            try {
                var did = new SP_DEVICE_INTERFACE_DATA(); did.cbSize = Marshal.SizeOf(did);
                for (int i = 0; SetupDiEnumDeviceInterfaces(hDev, IntPtr.Zero, ref GUID_DISK, i, ref did); i++) {
                    int req = 0; var info = new SP_DEVINFO_DATA(); info.cbSize = Marshal.SizeOf(info);
                    SetupDiGetDeviceInterfaceDetail(hDev, ref did, IntPtr.Zero, 0, ref req, ref info);
                    IntPtr det = Marshal.AllocHGlobal(req);
                    try {
                        Marshal.WriteInt32(det, IntPtr.Size == 8 ? 8 : 6);   // cbSize of SP_DEVICE_INTERFACE_DETAIL_DATA
                        info = new SP_DEVINFO_DATA(); info.cbSize = Marshal.SizeOf(info);
                        if (!SetupDiGetDeviceInterfaceDetail(hDev, ref did, det, req, ref req, ref info)) continue;
                        string path = Marshal.PtrToStringUni(new IntPtr(det.ToInt64() + 4));
                        if (DeviceNumberOf(path) != num) continue;
                        // our disk: eject it, or the first ejectable ancestor up the USB tree
                        var diag = new StringBuilder();
                        uint target = info.DevInst;
                        for (int up = 0; up < 8; up++) {
                            int veto = -1; var sb = new StringBuilder(300);
                            int cr = CM_Request_Device_EjectW(target, out veto, sb, 300, 0);
                            if (cr == 0 && veto == 0) return "OK";
                            diag.Append(String.Format("[cr={0} veto={1} {2}] ", cr, veto, sb.ToString()));
                            uint parent;
                            if (CM_Get_Parent(out parent, target, 0) != 0) break;
                            target = parent;
                        }
                        return "removal vetoed " + diag.ToString();
                    } finally { Marshal.FreeHGlobal(det); }
                }
                return "disk devnode not found for device number " + num;
            } finally { SetupDiDestroyDeviceInfoList(hDev); }
        }
    }
}
'@
    }
    $res = [Una.DeviceEject]::Eject([char]$letter)
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
    $bad = 0; $na = 0
    foreach ($a in $srcApps) {
        $appDir = Join-Path $appsRoot $a.App
        if (-not (Test-Path -LiteralPath $appDir -PathType Container)) { Write-Host ("  [n/a     ] {0} (not installed - skipped)" -f $a.App); $na++; continue }
        $dst = Join-Path $appDir $a.File.Name
        if (-not (Test-Path -LiteralPath $dst)) { Write-Host ("  [MISSING ] {0}\{1}" -f $a.App, $a.File.Name); $bad++; continue }
        if ((Get-FileHash -LiteralPath $a.File.FullName -Algorithm SHA256).Hash -eq (Get-FileHash -LiteralPath $dst -Algorithm SHA256).Hash) {
            Write-Host ("  [OK      ] {0}\{1}" -f $a.App, $a.File.Name)
        } else { Write-Host ("  [MISMATCH] {0}\{1}" -f $a.App, $a.File.Name); $bad++ }
    }
    if ($na -gt 0) { Write-Host ("  ($na source app(s) not installed on the watch - skipped)") }
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
        if (-not $exists) { [System.IO.Directory]::CreateDirectory($appDir) | Out-Null }   # .NET (no prompt in non-interactive)
        try { [System.IO.File]::Copy($a.File.FullName, $dst, $true) }          # new first, via .NET (not Copy-Item)
        catch {
            Write-Warning ("  {0}: copy failed ({1}); removing any partial file" -f $a.App, $_.Exception.Message)
            if (Test-Path -LiteralPath $dst) { [System.IO.File]::Delete($dst) }
            $failed += $a.App; continue
        }
        $dstLen = (Get-Item -LiteralPath $dst).Length
        if ($a.File.Length -ne $dstLen) {
            # bad copy: remove the corrupt new file; do NOT delete any stale .uapp
            [System.IO.File]::Delete($dst)
            $remain = @(Get-ChildItem -LiteralPath $appDir -Filter *.uapp -File)
            if ($remain.Count -eq 0) { Write-Warning ("  {0}: size mismatch - bad copy removed; NO valid .uapp remains, re-run to reinstall (settings/Activity preserved)" -f $a.App) }
            else { Write-Warning ("  {0}: size mismatch - bad copy removed; kept existing {1}" -f $a.App, $remain[0].Name) }
            $failed += $a.App; continue
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

if ($tmp) { Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue -WhatIf:$false -Confirm:$false }
