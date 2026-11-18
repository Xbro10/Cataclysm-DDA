param([string]$Out, [string]$Preview)
# Draws the Backrooms Wiki finds, UndeadPeople-style, 32x32 tiles in a row.
# 0 moth jelly (br_moth_jelly)       1 firesalt (br_firesalt)
# 2 royal ration (br_royal_ration)   3 cloudy water / liquid pain, loose (br_liquid_pain)
# 4 firesalt torch (br_firesalt_torch) 5 firesalt torch, lit (br_firesalt_torch_lit)
# 6 firesalt twist (br_firesalt_twist)
Add-Type -AssemblyName System.Drawing
$T = 32
$Count = 7
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

# ---- 4, 5: firesalt torch: a stick corner to corner, its head wrapped in rags crusted with red crystals.
$stick = Hex '#8a5a32'; $stickL = Hex '#b07a46'; $ragC = Hex '#b8ab90'; $ragD = Hex '#8a7e66'; $salt = Hex '#e04a2a'; $saltL = Hex '#ff8a5a'
foreach ($tt in 4, 5) {
  for ($k = 0; $k -le 17; $k++) { $x = 5 + $k; $y = 27 - $k; Px $tt $x $y $stick; Px $tt ($x + 1) $y $stick; Px $tt $x ($y - 1) $stickL }
  # Rag head: a fat wrapped knot at the top-right end.
  for ($k = 0; $k -le 6; $k++) { $cx = 21 + $k; $cy = 11 - $k; Fill $tt ($cx - 2) ($cy - 1) 4 3 $ragC; Px $tt ($cx + 1) ($cy + 1) $ragD }
  Line $tt 19 11 23 7 $ragD; Line $tt 22 12 26 8 $ragD
  foreach ($p in @(@(21, 9), @(23, 10), @(24, 6), @(26, 7), @(22, 7), @(25, 9), @(27, 5))) { Px $tt $p[0] $p[1] $salt }
  Px $tt 24 6 $saltL; Px $tt 21 9 $saltL
}
Outline 4; Shadow 4 15 29 9
# Lit: a red-orange flame rising from the head, embers.
$fR = Hex '#e0482a'; $fO = Hex '#ff9a32'; $fY = Hex '#ffe07a'
Ell 5 25 5 4 5 $fR; Ell 5 25 5 3 4 $fO; Ell 5 25 6 1 2 $fY; Ell 5 23 2 1 2 $fO; Ell 5 27 1 1 1 $fR
Px 5 25 0 $fR; Px 5 26 0 $fO
Outline 5; Px 5 20 1 $fO; Px 5 29 3 $fO; Shadow 5 15 29 9

# ---- 6: firesalt twist: a twisted paper wrap, ends screwed tight, red crystals showing through.
$pap = Hex '#ece4d0'; $papD = Hex '#c4b896'
Ell 6 16 20 6 3 $pap; Line 6 9 19 6 17 $papD; Line 6 9 21 6 22 $papD; Line 6 23 19 26 17 $papD; Line 6 23 21 26 22 $papD
Fill 6 6 17 2 6 $pap; Fill 6 25 17 2 6 $pap
Line 6 12 18 20 22 $papD
foreach ($p in @(@(14, 20), @(17, 19), @(18, 21), @(15, 22))) { Px 6 $p[0] $p[1] $salt }
Outline 6; Shadow 6 16 25 9

SaveSheet $Out $Preview $Count '#5c5650'
