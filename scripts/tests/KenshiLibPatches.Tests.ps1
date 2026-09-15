# Integration checks on isolated copies; never edits the fetched dependency.
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) '..\..'))
$fixture = Join-Path $repo ('build\kenshilib-patch-test-' + [Guid]::NewGuid().ToString('N'))
$source = Join-Path $repo 'third_party\KenshiLib_deps'
$apply = Join-Path $repo 'scripts\apply_kenshilib_patches.ps1'
$patchFile = Join-Path $repo 'third_party\kenshilib_patches\0001-canonical-building-designation.patch'
$relative = 'KenshiLib\Include\kenshi\Platoon.h'
$canonicalRelative = 'KenshiLib\Include\kenshi\Building\Building.h'
$target = Join-Path $fixture $relative
$canonical = Join-Path $fixture $canonicalRelative
$script:checks = 0
function Check([string]$Name, [bool]$Pass) {
    if (-not $Pass) { throw "FAIL: $Name" }
    $script:checks++
    Write-Output "PASS: $Name"
}
foreach ($rel in @($relative, $canonicalRelative)) {
    $dest = Join-Path $fixture $rel
    New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $source $rel) -Destination $dest
}
# Normalize the fixture to the tracked patched state, then restore its original
# source using the tracked inverse patch to test the first application too.
& powershell -NoProfile -ExecutionPolicy Bypass -File $apply -DependencyRoot $fixture
if ($LASTEXITCODE -ne 0) { throw 'Unsupported fixture source' }
& git -c "safe.directory=$repo" -C $fixture apply --reverse -- $patchFile
if ($LASTEXITCODE -ne 0) { throw 'Could not prepare original fixture' }
$canonicalBytes = [IO.File]::ReadAllText($canonical)
& powershell -NoProfile -ExecutionPolicy Bypass -File $apply -DependencyRoot $fixture
Check 'apply original snapshot' ($LASTEXITCODE -eq 0)
$patchedBytes = [IO.File]::ReadAllText($target)
Check 'canonical enum unchanged' ([IO.File]::ReadAllText($canonical) -ceq $canonicalBytes)
& powershell -NoProfile -ExecutionPolicy Bypass -File $apply -DependencyRoot $fixture
Check 'repeat apply succeeds' ($LASTEXITCODE -eq 0)
Check 'repeat apply changes no bytes' ([IO.File]::ReadAllText($target) -ceq $patchedBytes)
[IO.File]::AppendAllText($target, '// unexpected edit')
$modified = [IO.File]::ReadAllText($target)
$ErrorActionPreference = 'Continue' # PowerShell 5 wraps expected native stderr as errors.
& powershell -NoProfile -ExecutionPolicy Bypass -File $apply -DependencyRoot $fixture *> (Join-Path $fixture 'expected-source-rejection.log')
$ErrorActionPreference = 'Stop'
Check 'reject modified source' ($LASTEXITCODE -ne 0)
Check 'rejection preserves source' ([IO.File]::ReadAllText($target) -ceq $modified)
[IO.File]::WriteAllText($target, $patchedBytes)
[IO.File]::AppendAllText($canonical, '// unexpected canonical edit')
$ErrorActionPreference = 'Continue'
& powershell -NoProfile -ExecutionPolicy Bypass -File $apply -DependencyRoot $fixture *> (Join-Path $fixture 'expected-canonical-rejection.log')
$ErrorActionPreference = 'Stop'
Check 'reject changed canonical snapshot' ($LASTEXITCODE -ne 0)
Check 'canonical rejection leaves target untouched' ([IO.File]::ReadAllText($target) -ceq $patchedBytes)
Write-Output "KenshiLib patch fixtures: $script:checks/$script:checks passed. Artifacts: $fixture"
