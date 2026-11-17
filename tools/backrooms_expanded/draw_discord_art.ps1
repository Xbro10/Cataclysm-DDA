param([string]$OutDir)
# Xbro10 fork: draws the Discord "Playing" card picture (512x512 PNG) from the launcher's icon
# (tools/launcher/Launcher/launcher-icon.png). Upload it to the Discord application
# (Rich Presence > Art Assets) named "cdda_menu", the name data/discord_presence.json uses for every place.
Add-Type -AssemblyName System.Drawing
$root = Resolve-Path "$PSScriptRoot\..\.."
New-Item -ItemType Directory -Force $OutDir | Out-Null

$icon = [System.Drawing.Image]::FromFile("$root\tools\launcher\Launcher\launcher-icon.png")
$b = New-Object System.Drawing.Bitmap 512, 512
$g = [System.Drawing.Graphics]::FromImage($b)
$g.Clear([System.Drawing.Color]::FromArgb(255, 20, 19, 12))
$g.InterpolationMode = 'HighQualityBicubic'
$g.DrawImage($icon, 32, 32, 448, 448)
$b.Save("$OutDir\cdda_menu.png", [System.Drawing.Imaging.ImageFormat]::Png)
$b.Dispose(); $icon.Dispose()
"saved cdda_menu.png"
