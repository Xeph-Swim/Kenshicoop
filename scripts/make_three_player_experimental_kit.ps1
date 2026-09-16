<#
.SYNOPSIS
  Build the protocol-v56 experimental kit with one identical mod folder for
  every PC and Steam P2P selected by default.
#>
[CmdletBinding()]
param(
    [switch]$SkipBuild,
    [string]$HostDir = "C:\Program Files (x86)\Steam\steamapps\common\Kenshi"
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir

if (-not $SkipBuild) {
    & cmd.exe /c "`"$scriptDir\build_plugin.cmd`" Release"
    if ($LASTEXITCODE -ne 0) { throw "Release build failed ($LASTEXITCODE)" }
}

function Resolve-First([string[]]$Candidates, [string]$What) {
    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path $candidate)) { return $candidate }
    }
    throw "$What not found"
}

$dll = Resolve-First @(
    (Join-Path $repoRoot "src\plugin\x64\Release\KenshiCoop.dll"),
    (Join-Path $repoRoot "dist\mods\KenshiCoop\KenshiCoop.dll")
) "Release KenshiCoop.dll"
$json = Resolve-First @(
    (Join-Path $repoRoot "dist\mods\KenshiCoop\RE_Kenshi.json"),
    (Join-Path $HostDir "mods\KenshiCoop\RE_Kenshi.json")
) "RE_Kenshi.json"
$mod = Resolve-First @(
    (Join-Path $repoRoot "dist\mods\KenshiCoop\KenshiCoop.mod"),
    (Join-Path $HostDir "mods\KenshiCoop\KenshiCoop.mod")
) "KenshiCoop.mod"

$root = Join-Path $repoRoot "dist\three-player-experimental"
if (Test-Path $root) { Remove-Item -Recurse -Force $root }
New-Item -ItemType Directory -Force -Path $root | Out-Null

$folder = Join-Path $root "KenshiCoop"
New-Item -ItemType Directory -Force -Path $folder | Out-Null
Copy-Item $dll (Join-Path $folder "KenshiCoop.dll")
Copy-Item $json (Join-Path $folder "RE_Kenshi.json")
Copy-Item $mod (Join-Path $folder "KenshiCoop.mod")
@'
{
  // Protocol-v56 experiment. Install this exact folder on every PC.
  // Role, squad ownership, and the other player's Steam ID are selected in
  // the F2 panel for each session; nothing here is machine-specific.
  "transport": "steam",
  "maxPlayers": 3,
  "saveSync": true,
  "loadSync": true,
  "speedSync": false,
  "timeSync": false,
  "camInterest": false,
  "autoConnect": false
}
'@ | Set-Content (Join-Path $folder "coop_config.json") -Encoding UTF8

Copy-Item (Join-Path $repoRoot "docs\THREE_PLAYER_EXPERIMENT.md") `
          (Join-Path $root "README.md")

$sha = (Get-FileHash -Algorithm SHA256 $dll).Hash
$protoLine = Select-String -Path (Join-Path $repoRoot "src\netproto\Wire.h") `
    -Pattern 'PROTOCOL_VERSION\s*=\s*(\d+)' | Select-Object -First 1
$proto = if ($protoLine) { $protoLine.Matches[0].Groups[1].Value } else { "?" }
@{
    dllSha256 = $sha
    protocolVersion = $proto
    builtUtc = (Get-Date).ToUniversalTime().ToString("o")
    config = "Release"
    transport = "steam"
    layout = "single-mod-folder"
    validationTarget = "host + two clients (UDP automated; Steam P2P setup only)"
} | ConvertTo-Json | Set-Content (Join-Path $root "PROVENANCE.json") -Encoding UTF8

if (@(Get-ChildItem $root -Filter KenshiCoop.dll -Recurse).Count -ne 1) {
    throw "Expected exactly one packaged KenshiCoop.dll in the shared mod folder"
}
$copy = Get-ChildItem $root -Filter KenshiCoop.dll -Recurse | Select-Object -First 1
if ((Get-FileHash -Algorithm SHA256 $copy.FullName).Hash -ne $sha) {
    throw "Packaged DLL differs from canonical Release DLL: $($copy.FullName)"
}

$zip = Join-Path $repoRoot "dist\KenshiCoop-3player-steam-experimental.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
$legacyZip = Join-Path $repoRoot "dist\KenshiCoop-3player-experimental.zip"
if (Test-Path $legacyZip) { Remove-Item $legacyZip -Force }
Compress-Archive -Path (Join-Path $root "*") -DestinationPath $zip

Write-Host "Three-player Steam experimental kit: $zip"
Write-Host "Release DLL SHA-256: $sha"
Write-Host "Protocol: v$proto"
