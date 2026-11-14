param([string]$Out, [string]$Preview)
# Draws the Backrooms Expanded entity sprite sheet: 32x32 tiles in a row, facing right (the game mirrors them).
# Original art in the style of UndeadPeople/UltiCa monsters: side view, standing on the bottom row,
# 2-3 shades per colour and a 1 px near-black outline added automatically around each figure.
# 0 Smiler, 1 Hound, 2 Deathmoth, 3 Wretch.
Add-Type -AssemblyName System.Drawing
$T = 32
$Count = 4
$bmp = New-Object System.Drawing.Bitmap ($Count * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)

function Hex([string]$hex) { [System.Drawing.ColorTranslator]::FromHtml($hex) }
$OUTLINE = Hex '#150f10'

function Px($tile, $x, $y, $col) { if ($x -ge 0 -and $x -lt $T -and $y -ge 0 -and $y -lt $T) { $bmp.SetPixel($tile * $T + $x, $y, $col) } }
function Fill($tile, $x, $y, $w, $h, $col) { for ($i = $x; $i -lt $x + $w; $i++) { for ($j = $y; $j -lt $y + $h; $j++) { Px $tile $i $j $col } } }
function Ell($tile, $cx, $cy, $rx, $ry, $col) {
  for ($j = -$ry; $j -le $ry; $j++) { for ($i = -$rx; $i -le $rx; $i++) {
    if ((($i * $i) / [double]($rx * $rx + 0.5)) + (($j * $j) / [double]($ry * $ry + 0.5)) -le 1.0) { Px $tile ($cx + $i) ($cy + $j) $col } } }
}
# A line $w pixels thick (Bresenham, thickened downward/rightward).
function Line($tile, $x0, $y0, $x1, $y1, $col, $w = 1) {
  $dx = [Math]::Abs($x1 - $x0); $dy = -[Math]::Abs($y1 - $y0)
  $sx = if ($x0 -lt $x1) { 1 } else { -1 }; $sy = if ($y0 -lt $y1) { 1 } else { -1 }
  $err = $dx + $dy; $x = $x0; $y = $y0
  while ($true) {
    Fill $tile $x $y $w $w $col
    if ($x -eq $x1 -and $y -eq $y1) { break }
    $e2 = 2 * $err
    if ($e2 -ge $dy) { $err += $dy; $x += $sx }
    if ($e2 -le $dx) { $err += $dx; $y += $sy }
  }
}
# Outline: every transparent pixel touching (4-way) a painted pixel becomes the outline colour.
function Outline($tile) {
  $hits = New-Object System.Collections.Generic.List[object]
  for ($y = 0; $y -lt $T; $y++) { for ($x = 0; $x -lt $T; $x++) {
    if ($bmp.GetPixel($tile * $T + $x, $y).A -ne 0) { continue }
    foreach ($d in @(@(1,0), @(-1,0), @(0,1), @(0,-1))) {
      $nx = $x + $d[0]; $ny = $y + $d[1]
      if ($nx -ge 0 -and $nx -lt $T -and $ny -ge 0 -and $ny -lt $T -and $bmp.GetPixel($tile * $T + $nx, $ny).A -eq 255) { $hits.Add(@($x, $y)); break }
    }
  } }
  foreach ($h in $hits) { Px $tile $h[0] $h[1] $OUTLINE }
}
# Soft ground shadow, drawn after the outline so it isn't outlined.
function Shadow($tile, $cx, $cy, $rx) {
  $sh = [System.Drawing.Color]::FromArgb(70, 0, 0, 0)
  for ($i = -$rx; $i -le $rx; $i++) { if ($bmp.GetPixel($tile * $T + $cx + $i, $cy).A -eq 0) { Px $tile ($cx + $i) $cy $sh } }
}

# ---- 0: Smiler. A shadow in the shape of a person, long arms, and a grin that glows.
$k = Hex '#141018'; $kk = Hex '#2a2232'; $eye = Hex '#f4eec2'; $grin = Hex '#fbfbec'; $gum = Hex '#4a0c10'
Line 0 13 21 12 29 $k 2; Line 0 18 21 19 29 $k 2
Ell 0 16 17 4 6 $k
Line 0 12 13 8 26 $k 2; Line 0 20 13 23 26 $k 2
Line 0 8 27 7 29 $k; Line 0 9 27 9 29 $k; Line 0 23 27 22 29 $k; Line 0 24 27 25 29 $k
Ell 0 16 8 6 5 $k
Line 0 21 5 21 11 $kk; Line 0 19 15 19 21 $kk; Line 0 23 15 24 24 $kk
Px 0 13 7 $eye; Px 0 14 7 $eye; Px 0 18 7 $eye; Px 0 19 7 $eye
Px 0 10 9 $grin; Px 0 22 9 $grin
Fill 0 11 10 11 1 $grin
for ($x = 12; $x -le 20; $x++) { Px 0 $x 11 ($(if ($x % 2) { $gum } else { $grin })) }
Fill 0 13 12 7 1 $grin
Outline 0; Shadow 0 16 31 5

# ---- 1: Hound. A pale person running on all fours, head low, mouth open.
$s = Hex '#a3ab9c'; $sd = Hex '#757d70'; $hair = Hex '#2b2522'; $eyeR = Hex '#d8262a'; $tooth = Hex '#f0eadc'; $mouth = Hex '#3a1212'
Ell 1 14 20 8 4 $s
Fill 1 8 22 12 2 $sd
Line 1 8 21 5 29 $s 2; Line 1 11 22 12 29 $sd 2
Line 1 20 21 24 29 $s 2; Line 1 18 22 19 29 $sd 2
Fill 1 4 29 3 1 $sd; Fill 1 24 29 3 1 $s
Ell 1 24 18 4 3 $s
Fill 1 25 20 3 1 $mouth; Px 1 26 19 $tooth; Px 1 28 20 $tooth
Px 1 25 17 $eyeR
Line 1 21 15 27 15 $hair; Line 1 20 16 15 17 $hair; Line 1 22 14 25 13 $hair
Line 1 8 17 19 17 $sd
Outline 1; Shadow 1 15 31 9

# ---- 2: Deathmoth. A dog-sized moth, wings spread, eyespots, hovering above its shadow.
$wing = Hex '#857462'; $wingD = Hex '#54473a'; $ring = Hex '#d1b77c'; $spot = Hex '#1a1414'; $fur = Hex '#6a5848'; $furL = Hex '#9a8670'
Ell 2 9 12 7 6 $wing; Ell 2 23 12 7 6 $wing
Ell 2 10 19 5 4 $wingD; Ell 2 22 19 5 4 $wingD
Ell 2 8 11 2 2 $ring; Px 2 8 11 $spot
Ell 2 24 11 2 2 $ring; Px 2 24 11 $spot
Line 2 4 15 13 15 $wingD; Line 2 19 15 28 15 $wingD
Ell 2 16 15 2 7 $fur
Line 2 16 9 16 20 $furL
Ell 2 16 7 2 2 $fur
Line 2 15 5 12 1 $fur; Line 2 17 5 20 1 $fur
Outline 2; Shadow 2 16 30 6

# ---- 3: Wretch. A survivor who stayed too long: grey skin, a faded jacket, head hanging.
$skin = Hex '#9c9086'; $skinD = Hex '#766a62'; $jac = Hex '#3f5470'; $jacD = Hex '#2b3a4f'; $jeans = Hex '#4b5a7a'; $boot = Hex '#2a2420'; $hairG = Hex '#7a746e'; $hole = Hex '#241818'; $blood = Hex '#6e1c1c'
Fill 3 13 23 3 7 $jeans; Fill 3 17 23 3 7 $jeans
Fill 3 13 29 3 1 $boot; Fill 3 17 29 4 1 $boot
Fill 3 12 14 9 10 $jac; Fill 3 19 14 2 10 $jacD
Fill 3 10 15 2 10 $jac; Fill 3 10 25 2 2 $skin
Fill 3 21 15 2 9 $jacD; Fill 3 21 24 2 2 $skinD
Px 3 14 18 $blood; Px 3 15 19 $blood; Px 3 17 21 $blood
Ell 3 18 10 4 4 $skin
Fill 3 20 11 2 3 $skinD
Line 3 14 6 20 6 $hairG; Px 3 14 7 $hairG; Px 3 15 8 $hairG; Px 3 21 7 $hairG
Fill 3 18 9 1 2 $hole; Fill 3 20 9 1 2 $hole; Fill 3 19 12 2 1 $hole
Fill 3 15 14 4 1 $skinD; Line 3 16 15 16 22 $jacD
Outline 3; Shadow 3 16 31 5

$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)

if ($Preview) {
  $sc = 8
  $pv = New-Object System.Drawing.Bitmap ($bmp.Width * $sc + ($Count - 1) * $sc), ($T * $sc)
  $g = [System.Drawing.Graphics]::FromImage($pv)
  $g.Clear((Hex '#6b6864'))
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
  $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
  # Not $n or $t: PowerShell names are case-insensitive.
  for ($ti = 0; $ti -lt $Count; $ti++) {
    $src = New-Object System.Drawing.Rectangle ($ti * $T), 0, $T, $T
    $dst = New-Object System.Drawing.Rectangle ($ti * ($T * $sc + $sc)), 0, ($T * $sc), ($T * $sc)
    $g.DrawImage($bmp, $dst, $src, [System.Drawing.GraphicsUnit]::Pixel)
  }
  $pv.Save($Preview, [System.Drawing.Imaging.ImageFormat]::Png)
}
