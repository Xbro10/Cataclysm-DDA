# Builds "CDDA Fork Launcher.exe" (in tools\launcher) from LauncherStarter.cs, with Launcher\launcher.ico
# as its file icon. Uses the C# compiler that ships with Windows (.NET Framework 4), so nothing to install.
# Run it again after changing the icon or the .cs file, then commit the new .exe.
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$root = Split-Path $here -Parent
$csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if (-not (Test-Path -LiteralPath $csc)) { $csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe' }
$out = Join-Path $root 'CDDA Fork Launcher.exe'
$ico = Join-Path $root 'Launcher\launcher.ico'
& $csc /nologo /target:winexe /optimize+ /platform:anycpu "/win32icon:$ico" "/out:$out" `
    /reference:System.Windows.Forms.dll (Join-Path $here 'LauncherStarter.cs')
if ($LASTEXITCODE -ne 0) { throw "csc failed ($LASTEXITCODE)" }
"Built $out ($((Get-Item -LiteralPath $out).Length) bytes)"
