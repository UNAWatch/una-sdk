# Regenerate c/*.c and c/*.h from config.csv.
# Generated files intentionally carry a "+Custom" profile-version suffix when
# the existing generated profile headers already have that suffix. Explicit
# custom blocks can also be preserved by wrapping them with matching markers:
#   // BEGIN FIT CUSTOM BLOCK: block_name
#   ...
#   // END FIT CUSTOM BLOCK: block_name
# Requires FitGen.exe for profile 21.202 (copy from Garmin FIT SDK or fit-c-sdk).
# Usage (from this directory):
#   powershell -ExecutionPolicy Bypass -File regenerate-c-sdk.ps1

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$fitGen = Join-Path $here 'FitGen.exe'
$generatedSourceRoot = Join-Path $here 'c'

function Get-GeneratedFiles {
    param([string]$Root)

    Get-ChildItem -LiteralPath $Root -Recurse -File |
        Where-Object { $_.Extension -in @('.c', '.h') -and $_.FullName -notmatch '[/\\]_deps[/\\]' } |
        ForEach-Object { $_.FullName }
}

function Get-ProfileVersionSuffix {
    param([string[]]$Paths)

    foreach ($path in $Paths) {
        if (-not (Test-Path $path)) {
            continue
        }

        $contents = Get-Content -LiteralPath $path -Raw
        if ($contents -match '(?m)^// Profile Version = .+?(?<suffix>\+[A-Za-z0-9._-]+)\s*$') {
            return $Matches['suffix']
        }
    }

    return ''
}

function Get-CustomBlocks {
    param([string[]]$Paths)

    $blocks = @{}
    $pattern = '(?ms)^[ \t]*// BEGIN FIT CUSTOM BLOCK: (?<name>[A-Za-z0-9_.-]+).*?^[ \t]*// END FIT CUSTOM BLOCK: \k<name>.*?$'

    foreach ($path in $Paths) {
        if (-not (Test-Path $path)) {
            continue
        }

        $contents = Get-Content -LiteralPath $path -Raw
        $regexMatches = [regex]::Matches($contents, $pattern)
        if ($regexMatches.Count -eq 0) {
            continue
        }

        $blocks[$path] = @{}
        foreach ($match in $regexMatches) {
            $blocks[$path][$match.Groups['name'].Value] = $match.Value
        }
    }

    return $blocks
}

function Restore-CustomBlocks {
    param(
        [hashtable]$BlocksByPath
    )

    foreach ($path in $BlocksByPath.Keys) {
        if (-not (Test-Path $path)) {
            continue
        }

        $contents = Get-Content -LiteralPath $path -Raw
        foreach ($name in $BlocksByPath[$path].Keys) {
            $pattern = "(?ms)^[ \t]*// BEGIN FIT CUSTOM BLOCK: $([regex]::Escape($name)).*?^[ \t]*// END FIT CUSTOM BLOCK: $([regex]::Escape($name)).*?$"
            if ($contents -notmatch $pattern) {
                throw "$($path): custom block '$name' was not present after regeneration"
            }
            $contents = [regex]::Replace($contents, $pattern, [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $BlocksByPath[$path][$name] }, 1)
        }
        Set-Content -LiteralPath $path -Value $contents -NoNewline -Encoding ASCII
    }
}

function Apply-ProfileVersionSuffix {
    param(
        [string[]]$Paths,
        [string]$Suffix
    )

    if ([string]::IsNullOrWhiteSpace($Suffix)) {
        return
    }

    foreach ($path in $Paths) {
        if (-not (Test-Path $path)) {
            continue
        }

        $contents = Get-Content -LiteralPath $path -Raw
        $updated = [regex]::Replace(
            $contents,
            '(?m)^(// Profile Version = [^\r\n+]*?)(?:\+[A-Za-z0-9._-]+)?\s*$',
            "`$1$Suffix")
        if ($updated -ne $contents) {
            Set-Content -LiteralPath $path -Value $updated -NoNewline -Encoding ASCII
        }
    }
}

function Assert-ProfileVersionSuffix {
    param(
        [string[]]$Paths,
        [string]$Suffix
    )

    if ([string]::IsNullOrWhiteSpace($Suffix)) {
        return
    }

    $missing = @()
    foreach ($path in $Paths) {
        if (-not (Test-Path $path)) {
            continue
        }

        $contents = Get-Content -LiteralPath $path -Raw
        if ($contents -match '(?m)^// Profile Version = ' -and
            $contents -notmatch "(?m)^// Profile Version = .*?$([regex]::Escape($Suffix))\s*$") {
            $missing += $path
        }
    }

    if ($missing.Count -gt 0) {
        throw "Regeneration did not apply expected FIT profile suffix '$Suffix':`n$($missing -join "`n")"
    }
}

if (-not (Test-Path $fitGen)) {
    throw "FitGen.exe not found in $here. Copy the 21.202 FitGen.exe from the Garmin FIT SDK (or fit-c-sdk) into this folder."
}

Push-Location $here
try {
    $generatedFiles = @(Get-GeneratedFiles -Root $generatedSourceRoot)
    $profileSuffix = Get-ProfileVersionSuffix -Paths $generatedFiles
    $customBlocks = Get-CustomBlocks -Paths $generatedFiles

    & $fitGen -norewrite -c
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    $generatedFiles = @(Get-GeneratedFiles -Root $generatedSourceRoot)
    Restore-CustomBlocks -BlocksByPath $customBlocks
    Apply-ProfileVersionSuffix -Paths $generatedFiles -Suffix $profileSuffix

    Assert-ProfileVersionSuffix -Paths $generatedFiles -Suffix $profileSuffix
    Write-Host "Done. Profile headers report 21.202; verify record.step_length in c/fit_example.h"
}
finally {
    Pop-Location
}
