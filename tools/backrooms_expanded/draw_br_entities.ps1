param([string]$Out, [string]$Preview)
# Draws the Backrooms Expanded entity sprite sheet: 32x32 tiles in a row, facing right (the game mirrors them).
# Original art in the style of UndeadPeople/UltiCa monsters: side view, standing on the bottom row,
# 2-3 shades per colour and a 1 px near-black outline added automatically around each figure.
# 0 Smiler, 1 Hound, 2 Deathmoth, 3 Wretch, 4 Faceling, 5 Skin-Stealer, 6 Partygoer.
Add-Type -AssemblyName System.Drawing
$T = 32
$Count = 7
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

# ---- 3: Wretch. What's left of someone who stayed too long: no skin, all raw red muscle over bone,
# a skull face with glowing yellow eyes, ribs showing, long thin limbs, curved bone claws, pale growths.
$meat = Hex '#8e2a22'; $meatD = Hex '#5e1612'; $meatL = Hex '#b8483a'; $rib = Hex '#cc6a5a'
$bone = Hex '#dcd0b2'; $boneD = Hex '#a8987a'; $eyeY = Hex '#ffe066'; $glowY = Hex '#c89a2a'; $jaw = Hex '#2a0a08'; $growth = Hex '#e8dcc4'
# legs: long and thin, knobbly knees, splayed bony feet
Line 3 15 21 14 25 $meat 2; Line 3 14 25 13 29 $meat 2
Line 3 17 21 18 25 $meat 2; Line 3 18 25 19 29 $meat 2
Px 3 14 25 $meatD; Px 3 18 25 $meatD
Fill 3 12 30 3 1 $meatD; Fill 3 19 30 3 1 $meatD
# pelvis, sunken waist, ribcage
Ell 3 16 20 3 1 $meat
Fill 3 15 17 3 3 $meatD
Ell 3 16 13 3 3 $meat
Line 3 14 12 18 12 $rib; Line 3 14 14 18 14 $rib; Line 3 15 16 17 16 $rib
Line 3 16 11 16 16 $meatD
Px 3 18 13 $meatL; Px 3 18 15 $meatL
# arms: thin, hanging to the knees, ending in long curved bone claws
Line 3 13 11 11 17 $meat; Line 3 11 17 11 23 $meat
Line 3 19 11 21 17 $meat; Line 3 21 17 21 23 $meatL
Line 3 11 24 10 27 $bone; Px 3 9 28 $bone; Px 3 10 26 $boneD
Line 3 21 24 22 27 $bone; Px 3 23 28 $bone; Px 3 22 26 $boneD
# skull head on a thin neck: glowing eyes, gaping jaw
Line 3 16 8 16 10 $meatD
Ell 3 16 5 3 3 $meat
Px 3 18 4 $meatL; Px 3 18 5 $meatL
Px 3 15 4 $eyeY; Px 3 17 4 $eyeY; Px 3 15 5 $glowY; Px 3 17 5 $glowY
Fill 3 15 7 3 2 $jaw; Px 3 15 7 $bone; Px 3 17 7 $bone
# pale growths on the shoulder, hip and shins
Px 3 18 10 $growth; Px 3 19 11 $growth; Px 3 18 11 $boneD
Px 3 13 20 $growth; Px 3 13 21 $boneD
Px 3 14 27 $growth; Px 3 19 27 $growth; Px 3 18 28 $boneD
Outline 3; Shadow 3 16 31 5


# ---- 4: Faceling. An ordinary-looking person in plain clothes, except the face: smooth skin, nothing on it.
$fs = Hex '#d6ccbc'; $fsD = Hex '#ada190'; $fsL = Hex '#ece4d6'
$shirt = Hex '#6a7480'; $shirtD = Hex '#4b535e'; $pants = Hex '#3b3b44'; $pantsD = Hex '#2a2a31'; $shoe = Hex '#1e1c1e'
Line 4 14 20 14 29 $pantsD 2; Line 4 17 20 17 29 $pants 2
Fill 4 13 30 3 1 $shoe; Fill 4 17 30 4 1 $shoe
Fill 4 13 11 7 9 $shirt; Fill 4 13 11 1 9 $shirtD; Fill 4 13 19 7 1 $pantsD
Line 4 12 12 12 20 $shirtD; Px 4 12 21 $fsD; Px 4 12 22 $fsD
Line 4 19 12 20 20 $shirt; Px 4 20 21 $fs; Px 4 20 22 $fs
Fill 4 15 9 3 2 $fsD
Ell 4 16 5 3 4 $fs
Line 4 13 4 13 7 $fsD; Px 4 14 8 $fsD
Px 4 18 4 $fsL; Px 4 18 5 $fsL; Px 4 19 5 $fsL
Outline 4; Shadow 4 16 31 5

# ---- 5: Skin-Stealer. Tall, grey and sinewy, leaning forward on long legs, with a stolen human skin
# hanging loose off its shoulders and back (the slack face still on it) and long hooked fingers.
$gr = Hex '#6c6870'; $grD = Hex '#48444c'; $grL = Hex '#8e8a92'; $hide = Hex '#c49a80'; $hideD = Hex '#96705a'
$wht = Hex '#e8e4d8'; $maw = Hex '#200c0c'; $hook = Hex '#d8d0c0'
Line 5 15 19 13 24 $grD 2; Line 5 13 24 14 29 $grD 2
Line 5 18 19 20 24 $gr 2; Line 5 20 24 19 29 $gr 2
Fill 5 12 30 4 1 $grD; Fill 5 19 30 4 1 $gr
Ell 5 16 18 3 2 $gr
Line 5 16 17 18 8 $gr 3
Px 5 19 11 $grL; Px 5 19 13 $grL; Px 5 18 15 $grL
Line 5 17 9 14 15 $grD; Line 5 14 15 13 22 $grD; Px 5 13 23 $hook; Px 5 12 24 $hook
Ell 5 21 5 3 3 $gr; Fill 5 23 5 3 2 $gr
Px 5 22 4 $wht; Px 5 24 4 $wht; Line 5 21 7 25 7 $maw
Line 5 20 9 23 14 $gr; Line 5 23 14 24 21 $gr; Line 5 24 22 25 24 $hook; Px 5 26 24 $hook
# the stolen skin: a ragged flap from the shoulders down its back, a slack face on it
Ell 5 15 10 3 2 $hide
for ($j = 11; $j -le 21; $j++) { $hw = 5 - [int][Math]::Floor(($j - 11) / 3); Fill 5 (11 + ($j % 2)) $j $hw 1 $hide }
Px 5 12 22 $hideD; Px 5 14 22 $hideD; Px 5 11 20 $hideD
Px 5 12 14 $maw; Px 5 14 14 $maw; Line 5 12 17 14 17 $hideD; Px 5 13 15 $hideD
# its hair still on: a scalp hanging off the shoulder, strands down the back
$hair5 = Hex '#3a2a20'; Fill 5 13 8 4 1 $hair5; Px 5 12 9 $hair5; Line 5 11 10 11 13 $hair5; Px 5 12 10 $hair5
Px 5 12 15 $maw; Px 5 14 15 $maw
Line 5 21 10 22 13 $hideD
Outline 5; Shadow 5 17 31 6

# ---- 6: Partygoer. Tall and thin, the colour of old mustard, skin ridged at the neck, belly and knees.
# A small egg-shaped head with two dark slit eyes and a wide red grin; hands that end in mouths;
# legs that swell into thick stumps; a red balloon on a string beside it.
$och = Hex '#c8a238'; $ochD = Hex '#94741e'; $ochL = Hex '#e2c25a'; $slit = Hex '#2a1208'; $red = Hex '#8a1c14'; $teeth = Hex '#e8d8b0'
$bal = Hex '#d8302a'; $balD = Hex '#9c1c18'; $balL = Hex '#ff8a7a'; $str = Hex '#3a3440'
# legs: thin above the knee, swelling into thick stumps
Line 6 12 21 12 26 $och 2; Line 6 15 21 15 26 $och 2
Fill 6 10 26 4 4 $och; Fill 6 15 26 4 4 $och
Line 6 10 27 10 29 $ochD; Line 6 18 27 18 29 $ochD; Fill 6 10 30 4 1 $ochD; Fill 6 15 30 4 1 $ochD
Px 6 12 24 $ochD; Px 6 13 24 $ochD; Px 6 15 24 $ochD; Px 6 16 24 $ochD
# torso: shoulders, ribbed belly, narrow waist
Fill 6 11 12 7 4 $och; Px 6 10 12 $och; Px 6 18 12 $och; Fill 6 12 16 5 3 $och; Fill 6 12 19 5 2 $och
Line 6 12 17 16 17 $ochD; Line 6 13 19 15 19 $ochD; Line 6 14 13 14 15 $ochD
Line 6 12 13 13 13 $ochL; Line 6 15 13 16 13 $ochL
# long arms hanging past the hips, hands that are mouths
Line 6 8 12 6 21 $och 2; Line 6 19 12 21 21 $och 2
Fill 6 5 22 3 3 $och; Line 6 6 22 6 24 $red; Px 6 5 23 $teeth; Px 6 7 23 $teeth
Fill 6 21 22 3 3 $och; Line 6 22 22 22 24 $red; Px 6 21 23 $teeth; Px 6 23 23 $teeth
# ringed neck
Fill 6 12 10 5 1 $ochD; Fill 6 12 11 5 1 $och; Line 6 12 11 16 11 $ochD
# head: small egg, slit eyes, wide red grin
Ell 6 14 5 3 4 $och
Px 6 12 2 $ochL; Px 6 12 3 $ochL
Line 6 13 3 13 6 $slit; Line 6 15 3 15 6 $slit
Px 6 11 7 $slit; Px 6 17 7 $slit; Px 6 12 8 $slit; Line 6 13 8 15 8 $red; Px 6 16 8 $slit; Line 6 13 9 15 9 $slit
# the balloon, its string trailing down to the hand
Ell 6 27 5 3 4 $bal; Px 6 25 3 $balL; Px 6 25 4 $balL; Px 6 26 2 $balL; Px 6 29 7 $balD; Px 6 28 8 $balD
Px 6 27 10 $balD; Line 6 27 11 26 15 $str; Line 6 26 16 23 21 $str
Outline 6; Shadow 6 14 31 6

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
