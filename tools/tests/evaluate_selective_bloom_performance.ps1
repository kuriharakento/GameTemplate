param(
    [string]$ResultDirectory = ".\application\Resources\test-results\particles"
)

$ErrorActionPreference = "Stop"
$culture = [Globalization.CultureInfo]::InvariantCulture

function Read-KeyValueReport([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Missing timing report: $Path"
    }
    $values = @{}
    foreach ($line in Get-Content -LiteralPath $Path) {
        $separator = $line.IndexOf('=')
        if ($separator -gt 0) {
            $values[$line.Substring(0, $separator)] = $line.Substring($separator + 1)
        }
    }
    return $values
}

function Number($Report, [string]$Key) {
    return [double]::Parse($Report[$Key], $culture)
}

$off = Read-KeyValueReport (Join-Path $ResultDirectory "selective_bloom_gpu_timing_off.txt")
$legacy = Read-KeyValueReport (Join-Path $ResultDirectory "selective_bloom_gpu_timing_legacy.txt")
$materialOff = Read-KeyValueReport (Join-Path $ResultDirectory "selective_bloom_gpu_timing_material_off.txt")
$materialOn = Read-KeyValueReport (Join-Path $ResultDirectory "selective_bloom_gpu_timing_material_on.txt")

$reports = @($off, $legacy, $materialOff, $materialOn)
$conditionsMatch = ($reports | Where-Object {
    $_['status'] -ne 'PASS' -or $_['resolution'] -ne '1920x1080' -or $_['vsync'] -ne '0' -or
    $_['adapter'] -ne $off['adapter'] -or $_['driver_version'] -ne $off['driver_version']
}).Count -eq 0

$selectiveMs = Number $materialOn 'measured_total_avg_ms'
$legacyMs = Number $legacy 'measured_total_avg_ms'
$selectiveVsLegacyDeltaMs = $selectiveMs - $legacyMs
$selectiveVsLegacyPercent = if ($legacyMs -gt 0.0) { $selectiveVsLegacyDeltaMs / $legacyMs * 100.0 } else { [double]::PositiveInfinity }
$selectiveLegacyGate = $selectiveVsLegacyDeltaMs -le 1.0 -and $selectiveVsLegacyPercent -le 10.0

$particleMaskDeltaMs = (Number $materialOn 'mask_scene_avg_ms') - (Number $materialOff 'mask_scene_avg_ms')
$particleGate = $particleMaskDeltaMs -le 0.35
$offBlurSkipped = (Number $off 'blur_horizontal_avg_ms') -eq 0.0 -and (Number $off 'blur_vertical_avg_ms') -eq 0.0

# GBuffer RT3: RGBA8 -> RGBA16F (+4 B/px), Selective Mask: RGBA16F (+8 B/px).
# Existing Bright/Blur render targets and content-owned Emissive textures are outside this increment.
$bloomInfrastructureMiB = 1920.0 * 1080.0 * 12.0 / 1MB
$vramGate = $bloomInfrastructureMiB -le 32.0
$measuredPass = $conditionsMatch -and $selectiveLegacyGate -and $particleGate -and $offBlurSkipped -and $vramGate

$outputPath = Join-Path $ResultDirectory "selective_bloom_performance_evaluation.txt"
$lines = @(
    "measured_gates=$(if ($measuredPass) { 'PASS' } else { 'FAIL' })",
    "conditions_match=$(if ($conditionsMatch) { 1 } else { 0 })",
    "resolution=$($off['resolution'])",
    "vsync=$($off['vsync'])",
    "adapter=$($off['adapter'])",
    "driver_version=$($off['driver_version'])",
    "samples_per_case=$($off['samples'])",
    "off_measured_total_ms=$([string]::Format($culture, '{0:F6}', (Number $off 'measured_total_avg_ms')))",
    "legacy_measured_total_ms=$([string]::Format($culture, '{0:F6}', $legacyMs))",
    "selective_measured_total_ms=$([string]::Format($culture, '{0:F6}', $selectiveMs))",
    "selective_vs_legacy_delta_ms=$([string]::Format($culture, '{0:F6}', $selectiveVsLegacyDeltaMs))",
    "selective_vs_legacy_delta_percent=$([string]::Format($culture, '{0:F3}', $selectiveVsLegacyPercent))",
    "selective_vs_legacy_gate=$(if ($selectiveLegacyGate) { 'PASS' } else { 'FAIL' })",
    "particle_mask_delta_ms=$([string]::Format($culture, '{0:F6}', $particleMaskDeltaMs))",
    "particle_0_35ms_gate=$(if ($particleGate) { 'PASS' } else { 'FAIL' })",
    "bloom_off_blur_skipped=$(if ($offBlurSkipped) { 1 } else { 0 })",
    "bloom_infrastructure_vram_mib=$([string]::Format($culture, '{0:F3}', $bloomInfrastructureMiB))",
    "vram_32mib_gate=$(if ($vramGate) { 'PASS' } else { 'FAIL' })",
    "pre_feature_off_overhead_gate=NOT_MEASURED_NO_PRE_FEATURE_BINARY",
    "cpu_frame_delta_gate=NOT_MEASURED_NO_PRE_FEATURE_BINARY"
)
[IO.File]::WriteAllLines((Join-Path (Get-Location) $outputPath), $lines, [Text.UTF8Encoding]::new($false))
$lines

if (-not $measuredPass) { exit 1 }
