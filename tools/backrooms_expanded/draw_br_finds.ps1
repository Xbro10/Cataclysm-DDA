param([string]$Out, [string]$Preview)
# Draws the Backrooms Wiki finds, UndeadPeople-style, 32x32 tiles in a row.
# 0 moth jelly (br_moth_jelly)       1 firesalt (br_firesalt)
# 2 royal ration (br_royal_ration)   3 cloudy water / liquid pain, loose (br_liquid_pain)
Add-Type -AssemblyName System.Drawing
$T = 32
$Count = 4
$bmp = New-Object System.Drawing.Bitmap ($Count * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
$rng = New-Object System.Random 2323

# ---- 0: moth jelly: a pale, faintly glowing glob on a torn scrap of grey wing.
$wing = Hex '#8a8680'; $wingD = Hex '#64605a'; $wingL = Hex '#aaa69e'
$jel = Hex '#bfe8e4'; $jelL = Hex '#effffc'; $jelD = Hex '#86c4c0'; $glow = Hex '#dff8f4'
Ell 0 15 21 10 5 $wing; Grain 0 6 17 19 9 $wingD $wingL 22 $rng
Line 0 7 22 22 18 $wingD; Line 0 10 25 24 21 $wingD
Ell 0 16 17 6 5 $jel; Ell 0 17 19 4 2 $jelD; Ell 0 14 15 2 1 $jelL; Px 0 19 14 $glow; Px 0 12 18 $glow
Outline 0; Shadow 0 15 27 9

# ---- 1: firesalt: a little heap of red crystals with bright facets and a warm core.
$r1 = Hex '#e04a2a'; $r2 = Hex '#a82a1a'; $r3 = Hex '#ff8a5a'; $hot = Hex '#ffd27a'
$xtals = @(@(12, 20, 3), @(17, 19, 4), @(21, 21, 3), @(14, 23, 3), @(19, 23, 3), @(16, 16, 3), @(10, 23, 2), @(23, 24, 2))
foreach ($c in $xtals) {
  $cx = $c[0]; $cy = $c[1]; $s = $c[2]
  for ($i = 0; $i -lt $s; $i++) { Fill 1 ($cx - $i) ($cy - $s + 1 + $i) (1 + 2 * $i) 1 $r1 }
  Fill 1 ($cx - $s + 1) ($cy + 1) (2 * $s - 1) 1 $r2
  Px 1 $cx ($cy - $s + 1) $r3; Px 1 ($cx - 1) ($cy - $s + 2) $r3
}
Px 1 17 17 $hot; Px 1 12 19 $hot; Px 1 21 20 $hot
Outline 1; Shadow 1 16 26 9

# ---- 2: royal ration: a plain foil-wrapped bar, crimped ends, one gold band, no writing.
$foil = Hex '#c8ccd2'; $foilL = Hex '#eef0f4'; $foilD = Hex '#8e949c'; $gold = Hex '#d8b04a'; $goldD = Hex '#a07e2a'
Fill 2 7 13 18 9 $foil; Fill 2 7 13 18 2 $foilL; Fill 2 7 20 18 1 $foilD
for ($y = 13; $y -lt 22; $y += 2) { Px 2 6 $y $foilD; Px 2 25 $y $foilD; Px 2 5 $y $foil; Px 2 26 $y $foil }
Fill 2 14 13 4 8 $gold; Fill 2 14 20 4 1 $goldD; Px 2 15 14 (Hex '#f4d888')
Px 2 9 16 $foilL; Px 2 21 17 $foilL
Outline 2; Shadow 2 16 23 10

# ---- 3: cloudy water, loose: a milky puddle like spilled almond water, with a faint rust-pink tint at its edge.
$milk = Hex '#e8e2d2'; $milkD = Hex '#c8bca8'; $pink = Hex '#c89a8a'; $milkL = Hex '#fff8ee'
Ell 3 16 22 7 3 $milk; Ell 3 18 23 4 1 $milkD; Px 3 13 21 $milkL; Px 3 14 21 $milkL
Px 3 9 22 $pink; Px 3 23 22 $pink; Px 3 20 24 $pink; Px 3 11 23 $pink
Outline 3

SaveSheet $Out $Preview $Count '#5c5650'
