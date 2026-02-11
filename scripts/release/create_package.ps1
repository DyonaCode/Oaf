param(
    [string]$Version = "0.1.0",
    [string]$RuntimeId = ""
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Resolve-Path (Join-Path $ScriptDir "..\..")
$DistDir = Join-Path $RootDir "dist"
$PackageName = "oaflang-$Version"
$StagingDir = Join-Path $DistDir $PackageName

Write-Host "Creating release package '$PackageName'..."
if (Test-Path $StagingDir) {
    Remove-Item -Recurse -Force $StagingDir
}

New-Item -ItemType Directory -Force -Path (Join-Path $StagingDir "bin") | Out-Null

$publishArgs = @(
    (Join-Path $RootDir "OafLang.csproj"),
    "--configuration", "Release",
    "--output", (Join-Path $StagingDir "bin")
)

if ($RuntimeId -ne "") {
    $publishArgs += @("--runtime", $RuntimeId, "--self-contained", "false")
}

dotnet publish @publishArgs

$copyTargets = @(
    "docs",
    "examples",
    "SpecOverview.md",
    "SpecSyntax.md",
    "SpecRuntime.md",
    "SpecFileStructure.md",
    "SpecRoadmap.md"
)

foreach ($target in $copyTargets) {
    $sourcePath = Join-Path $RootDir $target
    if (Test-Path $sourcePath) {
        Copy-Item $sourcePath -Destination $StagingDir -Recurse -Force
    }
}

$readmePath = Join-Path $StagingDir "README.txt"
@"
OafLang Release Package $Version

Contents:
- bin/: Published CLI and runtime assets
- docs/: Guides and references
- examples/: Sample programs and tutorials
- Spec*.md: Language specification documents

Quick start:
1. Run 'dotnet --info' to verify your local SDK/runtime.
2. Execute '.\bin\OafLang.exe --self-test' to validate installation.
3. Compile source: '.\bin\OafLang.exe "flux x = 1; return x;" --bytecode'
"@ | Set-Content -Path $readmePath -Encoding UTF8

New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
$zipPath = Join-Path $DistDir "$PackageName.zip"
if (Test-Path $zipPath) {
    Remove-Item -Force $zipPath
}

Compress-Archive -Path $StagingDir -DestinationPath $zipPath -CompressionLevel Optimal
Write-Host "Package created: $zipPath"
