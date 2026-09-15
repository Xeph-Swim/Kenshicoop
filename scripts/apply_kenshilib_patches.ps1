<# Applies only verified KenshiLib compatibility patches; never fetches or resets dependencies. #>
[CmdletBinding()]
param([string]$DependencyRoot = '')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Get-SourceHash([string]$Path) {
    # Git checkouts may use LF or CRLF. Otherwise require exact source bytes.
    $source = [IO.File]::ReadAllText($Path).Replace("`r`n", "`n")
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($source)))).Replace('-', '').ToLowerInvariant() }
    finally { $sha.Dispose() }
}

try {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
    $repo = [IO.Path]::GetFullPath((Join-Path $scriptDir '..'))
    if (-not $DependencyRoot) { $DependencyRoot = Join-Path $repo 'third_party\KenshiLib_deps' }
    $dependency = (Resolve-Path -LiteralPath $DependencyRoot).Path
    $patchDir = Join-Path $repo 'third_party\kenshilib_patches'
    $manifest = Get-Content -LiteralPath (Join-Path $patchDir 'manifest.json') -Raw | ConvertFrom-Json
    # Preflight EVERY expected file before applying anything. Post hashes make
    # a second run a no-op, and reject unrelated/manual edits even on a git HEAD
    # whose version looks right. The canonical definition must stay untouched.
    $pending = @()
    foreach ($patch in $manifest.patches) {
        foreach ($guard in $patch.guards) {
            if ((Get-SourceHash (Join-Path $dependency $guard.path)) -ne $guard.sha256) {
                throw "Unsupported KenshiLib source snapshot: $($guard.path). Expected $($manifest.snapshot). No patch applied."
            }
        }
        $actual = Get-SourceHash (Join-Path $dependency $patch.path)
        if ($actual -eq $patch.afterSha256) {
            Write-Output "Already applied: $($patch.file)"
        } elseif ($actual -eq $patch.beforeSha256) {
            $patchFile = Join-Path $patchDir $patch.file
            & git -c "safe.directory=$repo" -c "safe.directory=$dependency" -C $dependency apply --check -- $patchFile
            if ($LASTEXITCODE -ne 0) { throw "Patch context check failed: $($patch.file). No patch applied." }
            $pending += $patch
        } else {
            throw "Unsupported or modified KenshiLib source: $($patch.path) (SHA256 $actual). Expected $($manifest.snapshot), original or tracked patched state. No patch applied."
        }
    }
    foreach ($patch in $pending) {
        & git -c "safe.directory=$repo" -c "safe.directory=$dependency" -C $dependency apply -- (Join-Path $patchDir $patch.file)
        if ($LASTEXITCODE -ne 0) { throw "Failed applying $($patch.file). Inspect dependency state before retrying." }
        if ((Get-SourceHash (Join-Path $dependency $patch.path)) -ne $patch.afterSha256) {
            throw "Post-apply verification failed: $($patch.path)."
        }
        Write-Output "Applied and verified: $($patch.file)"
    }
    exit 0
} catch {
    Write-Error -ErrorAction Continue "KenshiLib compatibility setup failed: $_"
    exit 1
}
