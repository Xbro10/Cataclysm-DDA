param([string]$Out, [string]$Preview)
# Draws the Level 2 overmap sprite sheet: seven 32x32 tiles in a row.
# 0-1 tunnel variants, 2 service ladder, 3 boiler room, 4 valve junction, 5 flooded dip, 6 Tunnel Crew camp.
# One map square is 24 tiles; at 32 px each corridor line (tiles 3-5, 11-13, 19-21) sits at px 4-7, 14-17, 25-28.
Add-Type -AssemblyName System.Drawing
$T = 32
$N = 7
$bmp = New-Object System.Drawing.Bitmap ($N * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$rng = New-Object System.Random 2002

function Hex([string]$hex) { [System.Drawing.ColorTranslator]::FromHtml($hex) }
function DrawPx($tile, $x, $y, $col) { if ($x -ge 0 -and $x -lt $T -and $y -ge 0 -and $y -lt $T) { $bmp.SetPixel($tile * $T + $x, $y, $col) } }
function DrawRect($tile, $x, $y, $w, $h, $col) { for ($i = $x; $i -lt $x + $w; $i++) { for ($j = $y; $j -lt $y + $h; $j++) { DrawPx $tile $i $j $col } } }

$wall = Hex '#2e2a26'; $wallSpeck = Hex '#3a352f'; $floor = Hex '#6b6864'; $floorSpeck = Hex '#5e5b57'
$copper = Hex '#a8693a'; $iron = Hex '#4f5559'; $bulb = Hex '#f0b84a'; $bulbGlow = Hex '#7a6440'

function Walls($tile) {
  DrawRect $tile 0 0 $T $T $wall
  for ($k = 0; $k -lt 60; $k++) { DrawPx $tile ($rng.Next(0, $T)) ($rng.Next(0, $T)) $wallSpeck }
}
# A corridor piece: horizontal ($h = $true) at row band $b from x0 to x1, or vertical at column band $b from y0 to y1.
function Corr($tile, $h, $b, $a0, $a1) {
  if ($h) { DrawRect $tile $a0 $b ($a1 - $a0) 4 $floor; for ($i = $a0; $i -lt $a1; $i += 3) { DrawPx $tile $i $b $copper; DrawPx $tile $i ($b + 3) $iron } }
  else { DrawRect $tile $b $a0 4 ($a1 - $a0) $floor; for ($j = $a0; $j -lt $a1; $j += 3) { DrawPx $tile $b $j $copper; DrawPx $tile ($b + 3) $j $iron } }
}
function Cross($tile) { Corr $tile $true 14 0 $T; Corr $tile $false 14 0 $T }
function Bulb($tile, $x, $y) { DrawRect $tile ($x - 1) ($y - 1) 3 3 $bulbGlow; DrawPx $tile $x $y $bulb }

# 0: tunnels, a few side corridors
Walls 0; Cross 0; Corr 0 $true 4 0 14; Corr 0 $false 25 14 32; Corr 0 $true 25 18 32; Bulb 0 15 15
# 1: tunnels, a different set of side corridors and a dead end
Walls 1; Cross 1; Corr 1 $false 4 0 18; Corr 1 $true 25 0 14; Corr 1 $true 4 18 30; Bulb 1 26 15

# 2: service ladder: rungs in a nook off the cross, under a bulb
Walls 2; Cross 2; Corr 2 $true 4 18 32
$rung = Hex '#c9ced4'; $hatch = Hex '#9aa3ab'
DrawRect 2 9 9 5 5 $floor; DrawRect 2 10 8 3 1 $hatch
for ($j = 9; $j -lt 14; $j += 2) { DrawRect 2 10 $j 3 1 $rung }
DrawPx 2 10 9 $rung; DrawPx 2 12 9 $rung; Bulb 2 12 12

# 3: boiler room: north-west quarter full of red-hot boilers
Walls 3; Cross 3; Corr 3 $false 25 0 32
$room = Hex '#5a4a40'; $boiler = Hex '#8a2a1e'; $boilerHot = Hex '#e0552a'; $roomWall = Hex '#1f1b18'
DrawRect 3 0 0 14 14 $roomWall; DrawRect 3 1 1 12 12 $room
foreach ($p in @(@(2,2), @(9,2), @(2,9), @(9,9))) { DrawRect 3 $p[0] $p[1] 3 3 $boiler; DrawPx 3 ($p[0] + 1) ($p[1] + 1) $boilerHot }
DrawRect 3 13 5 1 4 $room; DrawRect 3 5 13 4 1 $room; Bulb 3 7 7

# 4: valve junction: a red wheel at the crossing, steam joints around it
Walls 4; Cross 4; Corr 4 $true 25 0 18
$wheel = Hex '#d23a2a'; $steam = Hex '#d8dde0'
DrawRect 4 13 13 6 6 $floor
DrawPx 4 16 13 $wheel; DrawPx 4 16 18 $wheel; DrawPx 4 13 16 $wheel; DrawPx 4 18 16 $wheel
DrawPx 4 14 14 $wheel; DrawPx 4 17 14 $wheel; DrawPx 4 14 17 $wheel; DrawPx 4 17 17 $wheel; DrawRect 4 15 15 2 2 $wheel
foreach ($p in @(@(15,4), @(16,26), @(5,16), @(26,15))) { DrawPx 4 $p[0] $p[1] $steam; DrawPx 4 ($p[0] + 1) ($p[1] - 1) $steam }

# 5: flooded dip: the crossing under brown water
Walls 5; Cross 5; Corr 5 $false 4 14 32
$water = Hex '#4a5a52'; $waterHi = Hex '#6f8478'
DrawRect 5 8 14 16 4 $water; DrawRect 5 14 8 4 16 $water
for ($k = 0; $k -lt 10; $k++) { DrawPx 5 ($rng.Next(9, 23)) ($rng.Next(15, 17)) $waterHi; DrawPx 5 ($rng.Next(15, 17)) ($rng.Next(9, 23)) $waterHi }

# 6: Tunnel Crew camp: a warm-lit room with bunks, a drip pool and crates
Walls 6; Cross 6; Corr 6 $true 25 18 32
$warm = Hex '#7d6448'; $bunk = Hex '#4f5f7a'; $crate = Hex '#6e5a3a'; $green = Hex '#6fd06f'; $pool = Hex '#3f5868'
DrawRect 6 0 0 14 14 $roomWall; DrawRect 6 1 1 12 12 $warm
DrawRect 6 2 2 3 2 $bunk; DrawRect 6 9 2 3 2 $bunk; DrawRect 6 2 10 3 2 $bunk; DrawRect 6 9 10 3 2 $bunk
DrawRect 6 5 6 2 2 $crate; DrawRect 6 10 7 2 1 $pool
DrawRect 6 13 5 1 4 $green; DrawRect 6 5 13 4 1 $green; Bulb 6 7 4

$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)

if ($Preview) {
  $s = 8
  $pv = New-Object System.Drawing.Bitmap ($bmp.Width * $s + ($N - 1) * $s), ($T * $s)
  $g = [System.Drawing.Graphics]::FromImage($pv)
  $g.Clear((Hex '#1e1e1e'))
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
  $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
  # Not $n: PowerShell names are case-insensitive, so $n would overwrite $N.
  for ($ti = 0; $ti -lt $N; $ti++) {
    $src = New-Object System.Drawing.Rectangle ($ti * $T), 0, $T, $T
    $dst = New-Object System.Drawing.Rectangle ($ti * ($T * $s + $s)), 0, ($T * $s), ($T * $s)
    $g.DrawImage($bmp, $dst, $src, [System.Drawing.GraphicsUnit]::Pixel)
  }
  $pv.Save($Preview, [System.Drawing.Imaging.ImageFormat]::Png)
}
