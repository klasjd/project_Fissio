$desktop = [System.Environment]::GetFolderPath([System.Environment+SpecialFolder]::Desktop)
$projRoot = Split-Path -Parent $PSScriptRoot
if (Test-Path "$PSScriptRoot\build\fissio-gui.exe") {
    $projRoot = $PSScriptRoot
}

$wsh = New-Object -ComObject WScript.Shell
$sc = $wsh.CreateShortcut("$desktop\Fissio.lnk")
$sc.TargetPath = Join-Path $projRoot "build\fissio-gui.exe"
$sc.WorkingDirectory = Join-Path $projRoot "build"
$sc.Description = "Fissio ECH & WireGuard Controller"
$icoPath = Join-Path $projRoot "fissio_gui.ico"
if (Test-Path $icoPath) {
    $sc.IconLocation = "$icoPath,0"
}
$sc.Save()
Write-Host "Desktop shortcut created: $desktop\Fissio.lnk"
