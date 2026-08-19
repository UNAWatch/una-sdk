param([switch]$DryRun)

# Compute SDK root path
$sdkRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent

# Define the tools to find
$tools = @{
    "arm-none-eabi-gcc" = "STM32 Toolchain GCC"
    "make" = "Make utility"
    "cmake" = "CMake"
    "python" = "Python"
}

# Define base directories to search for tools
$baseSearchDirs = @(
    "$env:USERPROFILE\AppData\Local\Programs",
    "C:\Program Files (x86)",
    "C:\Program Files",
    "C:\ST",
    "E:\ST"
)

$foundPaths = @{}

# Find each tool
foreach ($tool in $tools.Keys) {
    try {
        $cmd = Get-Command $tool -ErrorAction Stop
        $foundPaths[$tool] = Split-Path $cmd.Source
        Write-Host "Found $($tools[$tool]): $($cmd.Source)"
    } catch {
        # Try to find in common locations
        $found = $false
        foreach ($baseDir in $baseSearchDirs) {
            if (Test-Path $baseDir) {
                $exePath = Get-ChildItem -Path $baseDir -Recurse -Filter "$tool.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
                if ($exePath) {
                    $foundPaths[$tool] = Split-Path $exePath.FullName
                    Write-Host "Found $($tools[$tool]): $($exePath.FullName)"
                    $found = $true
                    break
                }
            }
        }
        if (!$found) {
            Write-Host "Tool $($tools[$tool]) ($tool) not found in PATH or common locations."
        }
    }
}

# Prepare paths to add to PATH (directories containing the executables)
$pathsToAdd = $foundPaths.Values | Select-Object -Unique

# Get current user PATH
$currentPath = [Environment]::GetEnvironmentVariable("PATH", "User")

# Filter paths not already in PATH
$pathsToAdd = $pathsToAdd | Where-Object { $currentPath -notlike "*$_*" }

if ($DryRun) {
    Write-Host "Dry run mode: The following paths would be added to user PATH:"
    if ($pathsToAdd.Count -eq 0) {
        Write-Host "No new paths to add."
    } else {
        $pathsToAdd | ForEach-Object { Write-Host "  $_" }
    }
    Write-Host "Current PATH would become: $currentPath;$($pathsToAdd -join ';')"
    $sdkRootForward = $sdkRoot -replace '\\', '/'
    Write-Host "UNA_SDK would be set to: $sdkRootForward"
} else {
    if ($pathsToAdd.Count -gt 0) {
        $newPath = $currentPath + ";" + ($pathsToAdd -join ";")
        # Set the new PATH permanently for the user
        [Environment]::SetEnvironmentVariable("PATH", $newPath, "User")
        $env:PATH = $newPath
        Write-Host "Updated user PATH with new tool directories."
    } else {
        Write-Host "No new paths to add; PATH is already up to date."
    }
    # Set UNA_SDK environment variable with forward slashes
    $sdkRootForward = $sdkRoot -replace '\\', '/'
    [Environment]::SetEnvironmentVariable("UNA_SDK", $sdkRootForward, "User")
    $env:UNA_SDK = $sdkRootForward
    Write-Host "Set UNA_SDK to $sdkRootForward"
}
