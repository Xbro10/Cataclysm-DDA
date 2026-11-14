param([string]$Out, [string]$Preview)
# Draws the Level 0 furniture and item sheet, UndeadPeople-style, 32x32 tiles in a row.
# 0 water cooler (f_watercooler)        1 waycircle (f_waycircle)
# 2 rippling wall (f_br_thin_wall)       3 sealed concrete (f_br1_thin_wall)
# 4 almond water (almond_water)          5 almond water bottle (bottle_almond_water)
# The two wall pieces fill the tile like the Level 0 / Level 1 walls they sit in; the rest are outlined figures.
Add-Type -AssemblyName System.Drawing
$T = 32
$Count = 6
$bmp = New-Object System.Drawing.Bitmap ($Count * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
$rng = New-Object System.Random 404

# ---- 0: water cooler: white cabinet, a big blue jug upside down on top, red and blue taps.
$cab = Hex '#e4e2da'; $cabD = Hex '#b8b6ae'; $jug = Hex '#6aa8d8'; $jugL = Hex '#a8d4f0'; $jugD = Hex '#3f7aa8'
Fill 0 10 15 12 15 $cab; Fill 0 19 15 3 15 $cabD; Fill 0 11 26 10 1 $cabD
Fill 0 12 17 8 3 (Hex '#cfcdc4')
Px 0 13 18 (Hex '#d23a2a'); Px 0 18 18 (Hex '#2a6ad2')
Fill 0 14 13 4 2 $jugD
Ell 0 16 8 5 5 $jug; Fill 0 13 5 2 5 $jugL; Px 0 19 10 $jugD; Px 0 20 8 $jugD
Outline 0; Shadow 0 16 31 7

# ---- 1: waycircle: a squat boulder with a glyph circle painted on it, candles and a bone around it.
$rock = Hex '#8a8680'; $rockD = Hex '#64605a'; $rockL = Hex '#a8a49c'; $paint = Hex '#d8582a'; $paintD = Hex '#9a3a1a'
$wax = Hex '#efe6c8'; $flame = Hex '#ffc84a'; $boneC = Hex '#dcd0b2'
Ell 1 16 20 10 7 $rock
Ell 1 18 23 8 4 $rockD; Ell 1 13 16 5 3 $rockL; Ell 1 16 20 8 5 $rock
Grain 1 8 14 17 12 $rockD $rockL 25 $rng
for ($a = 0; $a -lt 360; $a += 20) { $r = [Math]::PI * $a / 180; Px 1 ([int](16 + 6 * [Math]::Cos($r))) ([int](19 + 4 * [Math]::Sin($r))) $paint }
Line 1 13 19 19 19 $paint; Line 1 16 16 16 22 $paintD; Px 1 14 17 $paintD; Px 1 18 21 $paintD
Fill 1 4 22 2 4 $wax; Px 1 4 21 $flame
Fill 1 26 20 2 4 $wax; Px 1 26 19 $flame
Line 1 22 28 27 27 $boneC
Outline 1; Shadow 1 16 29 10

# ---- 2: rippling wall (Level 0): the yellow wallpaper wall, but the stripes bend like cloth in a draft.
$top = Hex '#8e7c42'; $topD = Hex '#7a6a36'; $topL = Hex '#a8955a'; $face = Hex '#d9c46e'; $stripe = Hex '#a08a40'; $shine = Hex '#f4e8a8'; $base = Hex '#6e5a2e'
Fill 2 0 0 $T $T $top; Grain 2 0 0 $T 17 $topD $topL 30 $rng
Fill 2 0 17 $T 1 $INK; Fill 2 0 18 $T 14 $face
for ($x = 1; $x -lt $T; $x += 4) { for ($y = 18; $y -lt 29; $y++) { $off = [int]([Math]::Round(2.4 * [Math]::Sin(($y * 0.9 + $x * 0.4) / 1.8))); Px 2 ($x + $off) $y $stripe; Px 2 ($x + $off + 1) $y $stripe } }
for ($y = 19; $y -lt 28; $y += 4) { $sx = 2 + (($y * 7) % 18); for ($k = 0; $k -lt 6; $k++) { Px 2 ($sx + $k) ($y + [int]([Math]::Round(1.2 * [Math]::Sin($k / 1.5)))) $shine } }
Fill 2 0 29 $T 2 $base; Fill 2 0 31 $T 1 $INK

# ---- 3: sealed concrete (Level 1): a slab of newer, smoother concrete with pale fresh seams.
$con = Hex '#8e9094'; $conD = Hex '#787a7e'; $conL = Hex '#a2a4a8'; $new = Hex '#b6b8ba'; $newL = Hex '#c8cacc'; $seam = Hex '#5c5e62'
Fill 3 0 0 $T $T $con; Grain 3 0 0 $T $T $conD $conL 50 $rng
ORect 3 6 5 20 22 $new; Grain 3 6 5 20 22 $newL $newL 20 $rng
Line 3 6 16 25 16 $seam; Px 3 11 9 $seam; Px 3 12 10 $seam; Px 3 20 21 $seam; Px 3 21 22 $seam

# ---- 4: almond water: a small pale, milky puddle.
$milk = Hex '#efe6cf'; $milkD = Hex '#d4c8a8'; $milkL = Hex '#fffaf0'
Ell 4 16 22 7 3 $milk; Ell 4 18 23 4 1 $milkD; Px 4 13 21 $milkL; Px 4 14 21 $milkL
Outline 4

# ---- 5: almond water bottle: clear glass, milky water, white cap, a plain paper label.
$glass = Hex '#d8e6ea'; $glassD = Hex '#a8bcc2'; $cap = Hex '#f4f4f0'; $label = Hex '#c8a870'; $labelD = Hex '#9a7e4a'
Fill 5 14 8 4 3 $glass; Fill 5 14 6 4 2 $cap
Fill 5 12 11 8 14 $glass; Fill 5 13 14 6 10 $milk; Fill 5 18 14 1 10 $milkD
Fill 5 12 17 8 4 $label; Fill 5 12 20 8 1 $labelD; Fill 5 14 18 4 1 $labelD
Px 5 13 12 (Hex '#ffffff'); Px 5 13 13 (Hex '#ffffff')
Outline 5; Shadow 5 16 26 5

SaveSheet $Out $Preview $Count '#5c5650'
