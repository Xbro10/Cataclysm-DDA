param([string]$Out, [string]$Preview)
# Draws held-weapon overlays (what your character holds), 32x64 tiles like UltiCa's tall overlays
# (sprite_offset_y -32: the bottom 32 rows line up with the character's tile).
# 0 overlay_wielded_br_flamero_mado_off   1 overlay_wielded_br_flamero_mado_on
# The spear stands nearly upright at the character's side, like UltiCa's own pikes and spears.
Add-Type -AssemblyName System.Drawing
$W = 32; $H = 64
$Count = 2
$bmp = New-Object System.Drawing.Bitmap ($Count * $W), $H, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$T = 32
. "$PSScriptRoot\sprite_common.ps1"
# Tall tiles: replace the square-tile pixel helpers.
function Px($tile, $x, $y, $col) { if ($x -ge 0 -and $x -lt $W -and $y -ge 0 -and $y -lt $H) { $bmp.SetPixel($tile * $W + $x, $y, $col) } }
function Outline($tile) {
  $hits = New-Object System.Collections.Generic.List[object]
  for ($y = 0; $y -lt $H; $y++) { for ($x = 0; $x -lt $W; $x++) {
    if ($bmp.GetPixel($tile * $W + $x, $y).A -ne 0) { continue }
    foreach ($d in @(@(1, 0), @(-1, 0), @(0, 1), @(0, -1))) {
      $nx = $x + $d[0]; $ny = $y + $d[1]
      if ($nx -ge 0 -and $nx -lt $W -and $ny -ge 0 -and $ny -lt $H -and $bmp.GetPixel($tile * $W + $nx, $ny).A -eq 255) { $hits.Add(@($x, $y)); break }
    }
  } }
  foreach ($p in $hits) { Px $tile $p[0] $p[1] $INK }
}
$rng = New-Object System.Random 4646

$wood = Hex '#8a5a32'; $woodL = Hex '#b07a46'
$tank = Hex '#4a4e54'; $tankL = Hex '#7c828a'
$brass = Hex '#c89a3a'
$blade = Hex '#3a3c40'; $bladeL = Hex '#8e939a'
$heat = Hex '#b8402a'

# The shaft leans one pixel right every 14 rows, from the head (top) to the butt near the feet.
function ShaftX($y) { return 10 + [int][Math]::Floor(($y - 4) / 14) }
function HeldSpear($tile, $lit) {
  for ($y = 12; $y -le 58; $y++) { $x = ShaftX $y; Px $tile $x $y $wood; Px $tile ($x + 1) $y $woodL }
  # Fuel tank sleeve, low on the shaft, with the brass valve.
  for ($y = 34; $y -le 44; $y++) { $x = ShaftX $y; Px $tile ($x - 1) $y $tank; Px $tile $x $y $tankL; Px $tile ($x + 1) $y $tank; Px $tile ($x + 2) $y $tank }
  Px $tile ((ShaftX 38) + 3) 38 $brass; Px $tile ((ShaftX 39) + 3) 39 $brass
  # Butt cap.
  Px $tile (ShaftX 59) 59 $tank; Px $tile ((ShaftX 59) + 1) 59 $tank
  # Jet collar and the blackened head.
  Fill $tile 9 11 4 2 $tank; Px $tile 9 11 $brass; Px $tile 12 11 $brass
  $wd = @(0, 1, 1, 2, 2, 2, 1, 1)
  for ($k = 0; $k -lt 8; $k++) { $y = 3 + $k; for ($i = -$wd[$k]; $i -le $wd[$k] + 1; $i++) { Px $tile (10 + $i) $y $blade } ; Px $tile 10 $y $bladeL
    if ($lit -and $wd[$k] -gt 0) { Px $tile (10 - $wd[$k]) $y $heat; Px $tile (11 + $wd[$k]) $y $heat } }
  Px $tile 10 2 $bladeL
}

HeldSpear 0 $false
Outline 0

HeldSpear 1 $true
# Flame: a halo around the head, taller above than below (fire rises).
$f1 = Hex '#ffe27a'; $f2 = Hex '#ffa83a'; $f3 = Hex '#e8582a'; $f4 = Hex '#a8301e'
$head = New-Object System.Collections.Generic.List[object]
for ($y = 0; $y -lt 14; $y++) { for ($x = 4; $x -lt 18; $x++) { if ($bmp.GetPixel(1 * $W + $x, $y).A -ne 0) { $head.Add(@($x, $y)) } } }
$paint = @{}
for ($y = 0; $y -lt 18; $y++) { for ($x = 2; $x -lt 20; $x++) {
  if ($bmp.GetPixel(1 * $W + $x, $y).A -ne 0) { continue }
  $best = 99.0
  foreach ($p in $head) {
    $dx = $x - $p[0]; $dy = $y - $p[1]
    $dy2 = if ($dy -gt 0) { $dy * 2.2 } else { $dy * 0.6 }
    $d = [Math]::Sqrt($dx * $dx + $dy2 * $dy2)
    if ($d -lt $best) { $best = $d }
  }
  $v = $best + $rng.NextDouble() * 1.0
  if ($v -le 1.4) { $paint["$x,$y"] = $f1 } elseif ($v -le 2.3) { $paint["$x,$y"] = $f2 } elseif ($v -le 3.1) { $paint["$x,$y"] = $f3 } elseif ($v -le 3.7 -and $rng.NextDouble() -lt 0.6) { $paint["$x,$y"] = $f4 }
} }
foreach ($k in $paint.Keys) { $xy = $k.Split(','); Px 1 ([int]$xy[0]) ([int]$xy[1]) $paint[$k] }
Outline 1

# Save, plus an 8x preview with a gap between tiles.
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
if ($Preview) {
  $sc = 6
  $pv = New-Object System.Drawing.Bitmap ($bmp.Width * $sc + ($Count - 1) * $sc), ($H * $sc)
  $g = [System.Drawing.Graphics]::FromImage($pv)
  $g.Clear((Hex '#5c5650'))
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
  $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
  for ($ti = 0; $ti -lt $Count; $ti++) {
    $src = New-Object System.Drawing.Rectangle ($ti * $W), 0, $W, $H
    $dst = New-Object System.Drawing.Rectangle ($ti * ($W * $sc + $sc)), 0, ($W * $sc), ($H * $sc)
    $g.DrawImage($bmp, $dst, $src, [System.Drawing.GraphicsUnit]::Pixel)
  }
  $pv.Save($Preview, [System.Drawing.Imaging.ImageFormat]::Png)
}
