param([string]$Out, [string]$Preview)
# Draws what Backrooms entities leave behind, UndeadPeople-style, 32x32 tiles in a row.
# 0 hound fang (br_hound_fang)            1 Smiler's teeth (br_smiler_teeth)
# 2 cracked insulator (br_redeemer_insulator)  3 red balloon (br_red_balloon)
# 4 party hat (br_party_hat)
Add-Type -AssemblyName System.Drawing
$T = 32
$Count = 5
$bmp = New-Object System.Drawing.Bitmap ($Count * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
$rng = New-Object System.Random 5150

# ---- 0: hound fang: a long curved yellowed tooth, a darker root.
$ivory = Hex '#e8dcb0'; $ivoryD = Hex '#b8a878'; $ivoryL = Hex '#fff8e0'; $root = Hex '#8a6e4a'
for ($k = 0; $k -le 12; $k++) { $x = 10 + $k; $y = 22 - [int]([Math]::Round(0.05 * $k * $k)); $w = [Math]::Max(1, 4 - [int]($k / 4)); Fill 0 $x ($y - $w + 1) 1 ($w + 1) $ivory; Px 0 $x ($y + 1) $ivoryD }
Fill 0 8 20 3 4 $root; Px 0 9 21 (Hex '#6a5236')
Px 0 13 19 $ivoryL; Px 0 16 18 $ivoryL; Px 0 19 16 $ivoryL
Outline 0; Shadow 0 16 26 8

# ---- 1: Smiler's teeth: a crescent of white teeth set in a black, cold strip.
$gum = Hex '#18161c'; $gumL = Hex '#2e2a34'; $tooth = Hex '#f6f6f2'; $toothD = Hex '#c8c8c0'
# A grin: the strip curves up at both ends; teeth stand on it with dark gaps between.
for ($a = 15; $a -le 165; $a += 2) { $r = [Math]::PI * $a / 180; $x = [int](16 + 12 * [Math]::Cos($r)); $y = [int](12 + 9 * [Math]::Sin($r)); Fill 1 $x $y 2 4 $gum }
for ($a = 24; $a -le 156; $a += 16) { $r = [Math]::PI * $a / 180; $x = [int](16 + 12 * [Math]::Cos($r)); $y = [int](12 + 9 * [Math]::Sin($r)); Fill 1 $x ($y - 2) 2 3 $tooth; Px 1 $x ($y + 1) $toothD }
Px 1 6 15 $gumL; Px 1 25 15 $gumL
Outline 1; Shadow 1 16 26 11

# ---- 2: cracked insulator: a glazed brown ceramic bell with ribs, scorched and crazed.
$glaze = Hex '#8a4e24'; $glazeL = Hex '#c27a40'; $glazeD = Hex '#5a3014'; $scorch = Hex '#2a1a10'; $crack = Hex '#e8c890'
Ell 2 16 12 6 3 $glaze; Ell 2 16 11 4 1 $glazeL
Fill 2 13 12 7 6 $glaze
Ell 2 16 18 9 3 $glaze; Ell 2 16 21 10 3 $glaze; Ell 2 16 21 8 1 $glazeD; Ell 2 16 17 7 1 $glazeL
Fill 2 15 22 3 3 $glazeD
Line 2 12 13 14 19 $crack; Line 2 20 16 22 20 $crack
Px 2 18 13 $scorch; Px 2 19 14 $scorch; Px 2 11 19 $scorch; Px 2 21 22 $scorch
Outline 2; Shadow 2 16 26 10

# ---- 3: red balloon: a shiny red balloon on a string, the string curling down to the ground.
$red = Hex '#d8282a'; $redL = Hex '#ff6a5a'; $redD = Hex '#8e1418'; $string = Hex '#f0f0e8'
Ell 3 16 9 6 7 $red; Ell 3 17 11 4 4 $redD; Ell 3 15 8 4 5 $red; Px 3 13 5 $redL; Px 3 14 5 $redL; Px 3 13 6 $redL
Fill 3 15 16 3 1 $redD
Outline 3
for ($y = 18; $y -le 28; $y++) { $x = 16 + [int]([Math]::Round(2 * [Math]::Sin($y / 2.2))); Px 3 $x $y $string }
Shadow 3 16 29 4

# ---- 4: party hat: a striped yellow and red cone with a pom-pom and an elastic strap.
$yel = Hex '#f2c83a'; $yelD = Hex '#c09420'; $pr = Hex '#d8402a'; $pom = Hex '#ffffff'
for ($y = 7; $y -le 24; $y++) { $half = [int](($y - 7) * 0.45); for ($x = 16 - $half; $x -le 16 + $half; $x++) { $col = if ((([int](($x + $y) / 3)) % 2) -eq 0) { $yel } else { $pr }; Px 4 $x $y $col } }
for ($y = 8; $y -le 24; $y++) { $half = [int](($y - 7) * 0.45); Px 4 (16 + $half) $y $yelD }
Ell 4 16 6 2 2 $pom
Fill 4 8 25 17 1 (Hex '#e8e8e0')
Outline 4; Shadow 4 16 27 9

SaveSheet $Out $Preview $Count '#5c5650'
