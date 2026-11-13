param([string]$Out, [string]$Preview)
# Draws the Level 1 overmap sprite sheet: six 32x32 tiles in a row.
# 0-2 garage variants, 3 arrival alcove, 4 Breakroom, 5 squatter camp.
Add-Type -AssemblyName System.Drawing
$T = 32
$bmp = New-Object System.Drawing.Bitmap (6 * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$rng = New-Object System.Random 1337

function Hex([string]$hex) { [System.Drawing.ColorTranslator]::FromHtml($hex) }
function DrawPx($tile, $x, $y, $col) { if ($x -ge 0 -and $x -lt $T -and $y -ge 0 -and $y -lt $T) { $bmp.SetPixel($tile * $T + $x, $y, $col) } }
function DrawRect($tile, $x, $y, $w, $h, $col) { for ($i = $x; $i -lt $x + $w; $i++) { for ($j = $y; $j -lt $y + $h; $j++) { DrawPx $tile $i $j $col } } }
function Box($tile, $x, $y, $w, $h, $col) { DrawRect $tile $x $y $w 1 $col; DrawRect $tile $x ($y + $h - 1) $w 1 $col; DrawRect $tile $x $y 1 $h $col; DrawRect $tile ($x + $w - 1) $y 1 $h $col }

$concrete = Hex '#5f6266'; $speck1 = Hex '#55585c'; $speck2 = Hex '#6b6e72'
$line = Hex '#b89a2e'; $pillar = Hex '#2f3133'; $pillarHi = Hex '#45484b'
$lightCore = Hex '#f2ecc0'; $lightGlow = Hex '#8a8a78'; $deadLight = Hex '#3d3f42'
$puddle = Hex '#3f5868'; $puddleHi = Hex '#5f7f92'
$edge = Hex '#4b4e52'

function Floor($tile) {
  DrawRect $tile 0 0 $T $T $concrete
  for ($k = 0; $k -lt 70; $k++) { DrawPx $tile ($rng.Next(0, $T)) ($rng.Next(0, $T)) $speck1 }
  for ($k = 0; $k -lt 40; $k++) { DrawPx $tile ($rng.Next(0, $T)) ($rng.Next(0, $T)) $speck2 }
}
# Garage layout mirrors the real mapgen: two bands of bays (rows 0-5 and 12-17 of 24), lanes between.
function Bays($tile) {
  foreach ($x in 0, 4, 8, 12, 16, 20, 24, 28) { DrawRect $tile $x 1 1 7 $line; DrawRect $tile $x 17 1 7 $line }
  foreach ($x in 4, 12, 20, 28) { DrawRect $tile ($x - 1) 3 3 3 $pillar; DrawPx $tile $x 4 $pillarHi; DrawRect $tile ($x - 1) 19 3 3 $pillar; DrawPx $tile $x 20 $pillarHi }
}
function Light($tile, $x, $y, $on) {
  if ($on) { DrawRect $tile ($x - 2) ($y - 1) 5 3 $lightGlow; DrawRect $tile ($x - 1) $y 3 1 $lightCore }
  else { DrawRect $tile ($x - 1) $y 3 1 $deadLight }
}
function Car($tile, $x, $y, $body, $roof) { DrawRect $tile $x $y 3 6 $body; DrawRect $tile $x ($y + 2) 3 2 $roof; DrawPx $tile $x $y $roof; DrawPx $tile ($x + 2) $y $roof }

# 0: plain garage, lights on
Floor 0; Bays 0; Light 0 8 11 $true; Light 0 24 11 $true; Light 0 16 28 $true
# 1: garage with parked cars
Floor 1; Bays 1; Light 1 8 11 $true; Light 1 24 11 $false; Light 1 16 28 $true
Car 1 1 1 (Hex '#7a3b36') (Hex '#a35a52'); Car 1 21 17 (Hex '#3a4f6e') (Hex '#5a739a'); Car 1 9 17 (Hex '#6b6b5a') (Hex '#8e8e78')
# 2: dim garage: dead lights and puddles
Floor 2; Bays 2; Light 2 8 11 $false; Light 2 24 11 $false; Light 2 16 28 $true
DrawRect 2 10 12 5 2 $puddle; DrawRect 2 11 12 2 1 $puddleHi; DrawRect 2 22 26 4 2 $puddle; DrawRect 2 23 26 2 1 $puddleHi

# 3: arrival alcove: pipe-walled box with a rippling wall section (cyan) and one working light
Floor 3; Bays 3
$pipe = Hex '#3e4a40'; $pipeHi = Hex '#5c6e5e'; $ripple1 = Hex '#7fd6e0'; $ripple2 = Hex '#3fa3b5'
DrawRect 3 9 9 14 14 $concrete
Box 3 9 9 14 14 $pipe; Box 3 10 10 12 12 $pipeHi
DrawRect 3 14 22 4 1 $concrete; DrawRect 3 14 21 4 1 $concrete   # opening at the bottom
for ($i = 0; $i -lt 6; $i++) { DrawPx 3 (13 + $i) 9 ($(if ($i % 2) { $ripple1 } else { $ripple2 })); DrawPx 3 (13 + $i) 10 ($(if ($i % 2) { $ripple2 } else { $ripple1 })) }
Light 3 16 15 $true

# 4: the Breakroom: thick walls, warm lit floor, green door, sign arrow
Floor 4
$wall = Hex '#2b332b'; $wallHi = Hex '#47574a'; $warm = Hex '#8c7350'; $warm2 = Hex '#9e8560'; $green = Hex '#6fd06f'; $table = Hex '#5a4430'
DrawRect 4 3 3 26 20 $wall; DrawRect 4 5 5 22 16 $warm
for ($k = 0; $k -lt 40; $k++) { DrawPx 4 ($rng.Next(5, 27)) ($rng.Next(5, 21)) $warm2 }
Box 4 4 4 24 18 $wallHi
DrawRect 4 17 5 1 16 $wall; DrawRect 4 18 12 9 1 $wall             # inner walls: breakroom | bunks / stores
DrawRect 4 7 9 3 2 $table; DrawRect 4 12 9 3 2 $table; DrawRect 4 7 15 3 2 $table; DrawRect 4 12 15 3 2 $table
DrawRect 4 20 6 2 3 $table; DrawRect 4 24 6 2 3 $table               # bunks
DrawRect 4 20 15 6 4 (Hex '#6e5a3a')                            # crates
Light 4 10 12 $true; Light 4 22 16 $true
DrawRect 4 10 22 4 1 $green; DrawRect 4 10 23 4 1 $green             # door gap, marked green
DrawRect 4 10 26 4 1 $line; DrawRect 4 10 27 4 1 $line; DrawPx 4 11 25 $line; DrawPx 4 12 25 $line # sign arrow pointing up

# 5: squatter camp: sandbag ring, bedrolls, brazier glow
Floor 5; Bays 5
$sand = Hex '#a8915e'; $sandDk = Hex '#7d6a42'; $bed = Hex '#4f5f7a'; $fire = Hex '#ff8c1a'; $fireCore = Hex '#ffd27a'; $glow = Hex '#7a5a3a'
DrawRect 5 7 9 18 12 $concrete
Box 5 7 9 18 12 $sand; Box 5 8 10 16 10 $sandDk
DrawRect 5 14 9 4 1 $concrete; DrawRect 5 14 10 4 1 $concrete        # gap in the ring
DrawRect 5 13 13 6 5 $glow; DrawRect 5 15 14 2 3 $fire; DrawPx 5 15 15 $fireCore; DrawPx 5 16 15 $fireCore
DrawRect 5 9 12 2 4 $bed; DrawRect 5 21 12 2 4 $bed; DrawRect 5 9 17 3 2 $bed
DrawRect 5 20 17 2 2 (Hex '#6e5a3a')                            # stash crate
DrawRect 5 11 19 2 1 $puddle                                   # drip pool

$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)

if ($Preview) {
  # 8x nearest-neighbour preview with a 1px gap between tiles, for checking by eye.
  $s = 8
  $pv = New-Object System.Drawing.Bitmap ($bmp.Width * $s + 5 * $s), ($T * $s)
  $g = [System.Drawing.Graphics]::FromImage($pv)
  $g.Clear((Hex '#1e1e1e'))
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
  $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
  for ($n = 0; $n -lt 6; $n++) {
    $src = New-Object System.Drawing.Rectangle ($n * $T), 0, $T, $T
    $dst = New-Object System.Drawing.Rectangle ($n * ($T * $s + $s)), 0, ($T * $s), ($T * $s)
    $g.DrawImage($bmp, $dst, $src, [System.Drawing.GraphicsUnit]::Pixel)
  }
  $pv.Save($Preview, [System.Drawing.Imaging.ImageFormat]::Png)
}
