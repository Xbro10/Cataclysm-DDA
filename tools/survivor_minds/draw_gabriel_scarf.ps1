# Makes the blue scarf sprites for Survivor Minds' Gabriel (item sm_scarf_blue) by recolouring the
# wool scarf sprites of UltiCa and UndeadPeople (both CC-BY-SA 3.0) from brown to blue.
# Writes data/mods/Survivor_Minds/gfx/gabriel_*.png; gabriel_tileset_ultica.json and gabriel_tileset_udp.json map them.
# UltiCa isn't in the source checkout, so point -UltiCa at a release build's gfx/UltimateCataclysm.
param(
  [string]$UltiCa = 'D:\Games\CDDA MY FORK\Game\gfx\UltimateCataclysm',
  [string]$Udp = (Join-Path $PSScriptRoot '..\..\gfx\MSX++UnDeadPeopleEdition'),
  [string]$Out = (Join-Path $PSScriptRoot '..\..\data\mods\Survivor_Minds\gfx'),
  [double]$Hue = 212
)
Add-Type -AssemblyName System.Drawing

# Source sprites: sheet, x, y, width, height (found from each tileset's tile_config.json).
$sources = @{
  ultica_body = @{ dir = $UltiCa; file = 'human_body.png'; w = 32; h = 36; cells = @( @(416, 4860), @(384, 4860) ) }   # male, female overlay
  ultica_item = @{ dir = $UltiCa; file = 'small.png'; w = 20; h = 20; cells = @( , @(80, 880) ) }                          # item
  udp         = @{ dir = $Udp; file = 'normal_items.png'; w = 32; h = 32; cells = @( @(224, 22432), @(288, 22432), @(320, 22432) ) } # item, male overlay, overlay
}

function To-Blue([System.Drawing.Color]$c) {
  if ($c.A -eq 0) { return $c }
  $r = $c.R / 255.0; $g = $c.G / 255.0; $b = $c.B / 255.0
  $max = [Math]::Max($r, [Math]::Max($g, $b)); $min = [Math]::Min($r, [Math]::Min($g, $b))
  $v = $max; $s = if ($max -eq 0) { 0 } else { ($max - $min) / $max }
  if ($v -lt 0.16) { return $c }                       # keep the dark outline
  $s = [Math]::Min(1.0, [Math]::Max($s, 0.35) * 1.25)  # wool brown is dull; make the blue read as blue
  $h = $Hue / 60.0; $i = [Math]::Floor($h); $f = $h - $i
  $p = $v * (1 - $s); $q = $v * (1 - $s * $f); $t = $v * (1 - $s * (1 - $f))
  switch ([int]$i % 6) {
    0 { $rr = $v; $gg = $t; $bb = $p } 1 { $rr = $q; $gg = $v; $bb = $p } 2 { $rr = $p; $gg = $v; $bb = $t }
    3 { $rr = $p; $gg = $q; $bb = $v } 4 { $rr = $t; $gg = $p; $bb = $v } default { $rr = $v; $gg = $p; $bb = $q }
  }
  [System.Drawing.Color]::FromArgb($c.A, [int]($rr * 255), [int]($gg * 255), [int]($bb * 255))
}

New-Item -ItemType Directory -Force $Out | Out-Null
foreach ($name in $sources.Keys) {
  $src = $sources[$name]
  $sheet = New-Object System.Drawing.Bitmap (Join-Path $src.dir $src.file)
  $n = $src.cells.Count
  $bmp = New-Object System.Drawing.Bitmap ($src.w * $n), $src.h, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
  for ($k = 0; $k -lt $n; $k++) {
    $sx = $src.cells[$k][0]; $sy = $src.cells[$k][1]
    for ($x = 0; $x -lt $src.w; $x++) { for ($y = 0; $y -lt $src.h; $y++) {
      $bmp.SetPixel($k * $src.w + $x, $y, (To-Blue $sheet.GetPixel($sx + $x, $sy + $y)))
    } }
  }
  $sheet.Dispose()
  $bmp.Save((Join-Path $Out "gabriel_$name.png"), [System.Drawing.Imaging.ImageFormat]::Png)
  $bmp.Dispose()
  "wrote gabriel_$name.png ($n sprites)"
}
