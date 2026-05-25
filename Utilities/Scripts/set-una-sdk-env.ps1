# Sets the user-level UNA_SDK environment variable to this SDK submodule directory.
# Run from PowerShell:  . .\SDK\Utilities\Scripts\set-una-sdk-env.ps1

$sdkRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$sdkRootForward = $sdkRoot -replace '\\', '/'

if (-not (Test-Path -LiteralPath $sdkRoot)) {
    Write-Error "Computed SDK root does not exist: $sdkRootForward"
    return
}

$existingProcessValue = $env:UNA_SDK
$existingUserValue = [Environment]::GetEnvironmentVariable('UNA_SDK', 'User')
$existingValue = if ($existingProcessValue) { $existingProcessValue } else { $existingUserValue }

if ($existingValue -and $existingValue -ne $sdkRootForward) {
    Write-Warning "UNA_SDK is already set to: $existingValue"
    $answer = Read-Host "Overwrite UNA_SDK with $sdkRootForward? [y/N]"
    if ($answer -notin @('y', 'Y', 'yes', 'YES')) {
        Write-Host "UNA_SDK was not changed."
        return
    }
}

[Environment]::SetEnvironmentVariable('UNA_SDK', $sdkRootForward, 'User')
$env:UNA_SDK = $sdkRootForward

Write-Host "UNA_SDK set to: $sdkRootForward"
Write-Host "Restart Visual Studio and any open terminals for the change to take effect."
