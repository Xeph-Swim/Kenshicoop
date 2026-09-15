# Integration checks on isolated copies; never edits the fetched dependency.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repo = [IO.Path]::GetFullPath((Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) '..\..'))
$fixture = Join-Path $repo ('build\kenshilib-patch-test-' + [Guid]::NewGuid().ToString('N'))
$source = Join-Path $repo 'third_party\KenshiLib_deps'
$apply = Join-Path $repo 'scripts\apply_kenshilib_patches.ps1'
$patchDir = Join-Path $repo 'third_party\kenshilib_patches'
$manifest = Get-Content -LiteralPath (Join-Path $patchDir 'manifest.json') -Raw | ConvertFrom-Json
$script:checks = 0

function Check([string]$Name, [bool]$Pass) {
    if (-not $Pass) { throw "FAIL: $Name" }
    $script:checks++
    Write-Output "PASS: $Name"
}

function Get-SourceHash([string]$Path) {
    $sourceText = [IO.File]::ReadAllText($Path).Replace("`r`n", "`n")
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($sourceText)))).Replace('-', '').ToLowerInvariant()
    } finally {
        $sha.Dispose()
    }
}

$relativeFiles = @()
foreach ($patch in $manifest.patches) {
    $relativeFiles += [string]$patch.path
    foreach ($guard in $patch.guards) { $relativeFiles += [string]$guard.path }
}
foreach ($rel in ($relativeFiles | Sort-Object -Unique)) {
    $dest = Join-Path $fixture $rel
    New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $source $rel) -Destination $dest
}

# Normalize the fixture to the fully patched state, then restore every original
# source file using the tracked inverse patches. This supports a dependency tree
# where some patches were already applied by an earlier build.
& powershell -NoProfile -ExecutionPolicy Bypass -File $apply -DependencyRoot $fixture
if ($LASTEXITCODE -ne 0) { throw 'Unsupported fixture source' }
for ($i = $manifest.patches.Count - 1; $i -ge 0; $i--) {
    $patchFile = Join-Path $patchDir $manifest.patches[$i].file
    & git -C $fixture apply --reverse -- $patchFile
    if ($LASTEXITCODE -ne 0) { throw "Could not prepare original fixture for $patchFile" }
}

$guardBytes = @{}
foreach ($patch in $manifest.patches) {
    foreach ($guard in $patch.guards) {
        if (-not $guardBytes.ContainsKey([string]$guard.path)) {
            $guardBytes[[string]$guard.path] = [IO.File]::ReadAllText((Join-Path $fixture $guard.path))
        }
    }
}

& powershell -NoProfile -ExecutionPolicy Bypass -File $apply -DependencyRoot $fixture
Check 'apply original snapshot' ($LASTEXITCODE -eq 0)

$patchedBytes = @{}
foreach ($patch in $manifest.patches) {
    $target = Join-Path $fixture $patch.path
    $patchedBytes[[string]$patch.path] = [IO.File]::ReadAllText($target)
    Check "verified post hash: $($patch.file)" ((Get-SourceHash $target) -eq $patch.afterSha256)
}

$craftingPath = Join-Path $fixture 'KenshiLib\Include\kenshi\Building\CraftingBuilding.h'
$craftingText = [IO.File]::ReadAllText($craftingPath).Replace("`r`n", "`n")
$definitionAt = $craftingText.IndexOf("class CraftingItem`n{")
$dequeAt = $craftingText.IndexOf('std::deque<CraftingItem, std::allocator<CraftingItem> > crafting;')
Check 'CraftingItem is complete before deque instantiation' ($definitionAt -ge 0 -and $definitionAt -lt $dequeAt)

$guardsUnchanged = $true
foreach ($rel in $guardBytes.Keys) {
    if ([IO.File]::ReadAllText((Join-Path $fixture $rel)) -cne $guardBytes[$rel]) { $guardsUnchanged = $false }
}
Check 'guard definitions unchanged' $guardsUnchanged

& powershell -NoProfile -ExecutionPolicy Bypass -File $apply -DependencyRoot $fixture
Check 'repeat apply succeeds' ($LASTEXITCODE -eq 0)
$targetsUnchanged = $true
foreach ($rel in $patchedBytes.Keys) {
    if ([IO.File]::ReadAllText((Join-Path $fixture $rel)) -cne $patchedBytes[$rel]) { $targetsUnchanged = $false }
}
Check 'repeat apply changes no target bytes' $targetsUnchanged

foreach ($patch in $manifest.patches) {
    $target = Join-Path $fixture $patch.path
    [IO.File]::AppendAllText($target, '// unexpected edit')
    $modified = [IO.File]::ReadAllText($target)
    $log = Join-Path $fixture ("expected-source-rejection-$($patch.file).log")
    $ErrorActionPreference = 'Continue' # PowerShell 5 wraps expected native stderr as errors.
    & powershell -NoProfile -ExecutionPolicy Bypass -File $apply -DependencyRoot $fixture *> $log
    $rejected = $LASTEXITCODE -ne 0
    $ErrorActionPreference = 'Stop'
    Check "reject modified source: $($patch.file)" $rejected
    Check "rejection preserves source: $($patch.file)" ([IO.File]::ReadAllText($target) -ceq $modified)
    [IO.File]::WriteAllText($target, $patchedBytes[[string]$patch.path])
}

foreach ($rel in $guardBytes.Keys) {
    $guardPath = Join-Path $fixture $rel
    [IO.File]::AppendAllText($guardPath, '// unexpected guard edit')
    $beforeTargets = @{}
    foreach ($targetRel in $patchedBytes.Keys) {
        $beforeTargets[$targetRel] = [IO.File]::ReadAllText((Join-Path $fixture $targetRel))
    }
    $log = Join-Path $fixture 'expected-guard-rejection.log'
    $ErrorActionPreference = 'Continue'
    & powershell -NoProfile -ExecutionPolicy Bypass -File $apply -DependencyRoot $fixture *> $log
    $rejected = $LASTEXITCODE -ne 0
    $ErrorActionPreference = 'Stop'
    Check "reject changed guard snapshot: $rel" $rejected
    $targetsPreserved = $true
    foreach ($targetRel in $beforeTargets.Keys) {
        if ([IO.File]::ReadAllText((Join-Path $fixture $targetRel)) -cne $beforeTargets[$targetRel]) { $targetsPreserved = $false }
    }
    Check "guard rejection leaves targets untouched: $rel" $targetsPreserved
    [IO.File]::WriteAllText($guardPath, $guardBytes[$rel])
}

Write-Output "KenshiLib patch fixtures: $script:checks/$script:checks passed. Artifacts: $fixture"
