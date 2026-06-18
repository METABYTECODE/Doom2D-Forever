$paths = @(
    'src/engine',
    'src/game',
    'src/rivet_playground',
    'tests/test_engine.cpp',
    'tests/CMakeLists.txt',
    'CMakeLists.txt',
    'run_engine.bat'
)

$replacements = [ordered]@{
    'd2df_engine_playground' = 'rivet_playground'
    'd2df::engine ALIAS'     = 'rivet::engine ALIAS'
    'd2df::game ALIAS'       = 'rivet::game ALIAS'
    'd2df::engine::'         = 'rivet::'
    'namespace d2df::engine' = 'namespace rivet'
    '} // namespace d2df::engine' = '} // namespace rivet'
    'd2df::game::'           = 'rivet::game::'
    'namespace d2df::game'   = 'namespace rivet::game'
    '} // namespace d2df::game' = '} // namespace rivet::game'
    '<d2df/engine/'          = '<rivet/'
    '<d2df/game/'             = '<rivet/game/'
    'd2df_engine'             = 'rivet_engine'
    'd2df_game'               = 'rivet_game'
}

function Update-File {
    param([string]$FilePath)
    $content = Get-Content -LiteralPath $FilePath -Raw
    $original = $content
    foreach ($key in $replacements.Keys) {
        $content = $content.Replace($key, $replacements[$key])
    }
    if ($content -ne $original) {
        Set-Content -LiteralPath $FilePath -Value $content -NoNewline
    }
}

foreach ($path in $paths) {
    if (-not (Test-Path $path)) { continue }
    if (Test-Path $path -PathType Leaf) {
        Update-File $path
        continue
    }
    Get-ChildItem -LiteralPath $path -Recurse -File | ForEach-Object {
        Update-File $_.FullName
    }
}

Write-Host 'Rivet rename complete'
