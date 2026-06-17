<#
.SYNOPSIS
    Loop the Windows @mtls BDD scenario to reproduce the intermittent
    "oracle received 0 of 1 messages" flake and capture the client-side cause.

.DESCRIPTION
    Mirrors the bdd-windows-otel CI job locally on a native Windows box:
    starts otelcol-contrib with the BDD config, waits for the UDP 5514 /
    TCP 6514 / TCP 6515 listeners, then runs `behave` against the chosen tag
    in a loop. Each iteration's full output is kept; failing iterations are
    saved to Bdd/output/repro/iter-<NN>.log so the captured BDD-target stderr
    (added in ci/bdd-capture-target-stderr) is available to tell a real TLS
    failure (handshake timeout / cert rejected / fatal exit) from an
    environmental flake.

    Prerequisites:
      * The Windows BDD target is built, e.g.:
          cmake --preset msvc-debug
          cmake --build --preset msvc-debug --target SolidSyslogBddTarget
      * behave is installed (pip install behave) and on PATH.
      * Run from anywhere — the script resolves the repo root itself.

.PARAMETER Iterations
    Number of behave runs. Default 50.

.PARAMETER Tags
    behave --tags expression. Default '@mtls'. Use '@tls or @mtls' to widen,
    or the full Windows filter to exercise everything.

.PARAMETER Stress
    Spawn background CPU-burner threads for the duration to bias the runner
    toward the loaded conditions under which the flake appears.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\repro-mtls-flake.ps1 -Iterations 100 -Stress
#>
[CmdletBinding()]
param(
    [int]    $Iterations = 50,
    [string] $Tags       = '@mtls',
    [switch] $Stress
)

$ErrorActionPreference = 'Stop'

# Repo root = parent of this script's scripts/ directory.
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

$env:BDD_TARGET    = 'windows'
$env:EXAMPLE_BINARY = 'build/msvc-debug/Bdd/Targets/Debug/SolidSyslogBddTarget.exe'
$env:RECEIVED_LOG  = 'Bdd/output/received.jsonl'
$env:ORACLE_FORMAT = 'otel-jsonl'

if (-not (Test-Path $env:EXAMPLE_BINARY)) {
    throw "BDD target not found at $($env:EXAMPLE_BINARY). Build it first: cmake --preset msvc-debug; cmake --build --preset msvc-debug --target SolidSyslogBddTarget"
}

$otelBin    = Join-Path $repoRoot 'Bdd/otel/bin/otelcol-contrib.exe'
$otelConfig = Join-Path $repoRoot 'Bdd/otel/config.yaml'
if (-not (Test-Path $otelBin)) {
    throw "otelcol-contrib not found at $otelBin. Run Bdd/otel/Install-OtelCollector.ps1 first."
}

$outDir = Join-Path $repoRoot 'Bdd/output/repro'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

function Wait-OraclePorts {
    $deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $deadline) {
        $udp  = Get-NetUDPEndpoint   -LocalPort 5514 -ErrorAction SilentlyContinue
        $tcp4 = Get-NetTCPConnection -LocalPort 6514 -State Listen -ErrorAction SilentlyContinue
        $tcp5 = Get-NetTCPConnection -LocalPort 6515 -State Listen -ErrorAction SilentlyContinue
        if ($udp -and $tcp4 -and $tcp5) { return $true }
        Start-Sleep -Milliseconds 300
    }
    return $false
}

$stressJobs = @()
$otel = $null
try {
    Write-Host "Starting otelcol-contrib oracle..." -ForegroundColor Cyan
    Get-Process otelcol-contrib -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    $otel = Start-Process -FilePath $otelBin -ArgumentList "--config=$otelConfig" `
        -RedirectStandardOutput (Join-Path $outDir 'otelcol.out') `
        -RedirectStandardError  (Join-Path $outDir 'otelcol.err') `
        -PassThru -NoNewWindow

    if (-not (Wait-OraclePorts)) {
        throw "Oracle did not bind 5514/6514/6515 within 30s. See $outDir/otelcol.err"
    }
    Write-Host "Oracle listening on 5514/6514/6515." -ForegroundColor Green

    if ($Stress) {
        $n = [Environment]::ProcessorCount
        Write-Host "Spawning $n CPU-burner jobs for load..." -ForegroundColor Yellow
        $stressJobs = 1..$n | ForEach-Object {
            Start-Job { while ($true) { $x = 1; for ($i = 0; $i -lt 1000000; $i++) { $x = ($x * 7 + 13) % 1000003 } } }
        }
    }

    $fail = 0
    for ($i = 1; $i -le $Iterations; $i++) {
        $log = Join-Path $outDir ("iter-{0:D3}.log" -f $i)
        # Fresh oracle file each run so counts start from zero.
        Remove-Item $env:RECEIVED_LOG -ErrorAction SilentlyContinue
        $output = & behave --tags=$Tags --no-capture Bdd/features/ 2>&1
        $ok = ($LASTEXITCODE -eq 0)
        if ($ok) {
            Write-Host ("[{0,3}/{1}] PASS" -f $i, $Iterations) -ForegroundColor Green
        } else {
            $fail++
            $output | Out-File -FilePath $log -Encoding utf8
            Write-Host ("[{0,3}/{1}] FAIL -> {2}" -f $i, $Iterations, $log) -ForegroundColor Red
        }
    }

    Write-Host ""
    Write-Host ("Done: {0} failures in {1} runs." -f $fail, $Iterations) -ForegroundColor Cyan
    if ($fail -gt 0) {
        Write-Host "Inspect the failing logs for 'ERR:' lines (BDD target stderr) to see the client-side TLS cause." -ForegroundColor Cyan
    }
}
finally {
    if ($stressJobs) { $stressJobs | Remove-Job -Force -ErrorAction SilentlyContinue }
    Get-Process otelcol-contrib -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
}
