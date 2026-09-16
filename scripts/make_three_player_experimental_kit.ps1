<#
.SYNOPSIS
  Build the protocol-v56 host + two-client experimental direct-UDP kit.
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

function Add-PlayerFolder([string]$Name, [string]$Role, [string]$OwnSquads,
                          [string]$Ip) {
    $folder = Join-Path $root "$Name\KenshiCoop"
    New-Item -ItemType Directory -Force -Path $folder | Out-Null
    Copy-Item $dll (Join-Path $folder "KenshiCoop.dll")
    Copy-Item $json (Join-Path $folder "RE_Kenshi.json")
    Copy-Item $mod (Join-Path $folder "KenshiCoop.mod")
    @"
{
  // Protocol-v56 three-PC experiment. Keep these safety gates for the first run.
  "role": "$Role",
  "transport": "udp",
  "ip": "$Ip",
  "port": 27800,
  "maxPlayers": 3,
  "ownSquads": "$OwnSquads",
  "saveSync": false,
  "loadSync": false,
  "speedSync": false,
  "timeSync": false,
  "camInterest": false,
  "autoConnect": true
}
"@ | Set-Content (Join-Path $folder "coop_config.json") -Encoding UTF8
}

Add-PlayerFolder "Host"   "host" "0" "127.0.0.1"
Add-PlayerFolder "Join-A" "join" "1" "HOST_LAN_IP"
Add-PlayerFolder "Join-B" "join" "2" "HOST_LAN_IP"

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
    validationTarget = "host + two clients"
} | ConvertTo-Json | Set-Content (Join-Path $root "PROVENANCE.json") -Encoding UTF8

foreach ($copy in Get-ChildItem $root -Filter KenshiCoop.dll -Recurse) {
    if ((Get-FileHash -Algorithm SHA256 $copy.FullName).Hash -ne $sha) {
        throw "Packaged DLL differs from canonical Release DLL: $($copy.FullName)"
    }
}

$zip = Join-Path $repoRoot "dist\KenshiCoop-3player-experimental.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $root "*") -DestinationPath $zip

Write-Host "Three-player experimental kit: $zip"
Write-Host "Release DLL SHA-256: $sha"
Write-Host "Protocol: v$proto"
