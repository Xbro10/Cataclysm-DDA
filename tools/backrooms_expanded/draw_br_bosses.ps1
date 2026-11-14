param([string]$Out, [string]$Preview)
# Draws the Backrooms Expanded boss sprite sheet: 64x64 tiles (2x2 map tiles) in a row, facing right (the game mirrors them).
# Original art in UndeadPeople style; the figure stands on the bottom rows and the tileset offsets it to sit on its tile.
# 0 Redeemer: a huge three-headed dark-blue beast with a wooden power pole growing out of its back, wires and current crawling over it.
Add-Type -AssemblyName System.Drawing
$T = 64
$Count = 1
$bmp = New-Object System.Drawing.Bitmap ($Count * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
$rng = New-Object System.Random 64

# ---- 0: the Redeemer
# Palette: blue-black shaggy fur, steel-grey heads, a weathered brown pole with pale insulators, cyan current.
$fur = Hex '#1f2846'; $furD = Hex '#121831'; $furL = Hex '#34436f'; $furH = Hex '#4d5f93'
$head = Hex '#5c6478'; $headD = Hex '#3c4256'; $headL = Hex '#8a92a6'; $muzzle = Hex '#a9afbd'
$wood = Hex '#6b4a2c'; $woodD = Hex '#4a321e'; $woodL = Hex '#8c6640'; $ins = Hex '#d8dcd8'; $wire = Hex '#0e0e12'
$arc = Hex '#8ff0ff'; $arcW = Hex '#f4ffff'; $eye = Hex '#c8ffff'; $claw = Hex '#d8d4c8'; $tooth = Hex '#e8e6dc'

# Power pole growing out of the back: a tall pole, two cross-arms with insulators (behind everything else).
Fill 0 23 3 4 34 $wood; Fill 0 23 3 1 34 $woodL; Fill 0 26 3 1 34 $woodD
for ($y = 6; $y -lt 34; $y += 5) { Px 0 24 $y $woodD; Px 0 25 ($y + 2) $woodD }
Fill 0 14 8 22 2 $wood; Fill 0 14 8 22 1 $woodL; Fill 0 17 14 16 2 $wood; Fill 0 17 14 16 1 $woodL
Line 0 18 10 23 13 $woodD; Line 0 31 10 26 13 $woodD
foreach ($ix in 15, 20, 30, 35) { Fill 0 $ix 6 2 2 $ins }
foreach ($ix in 18, 32) { Fill 0 $ix 12 2 2 $ins }
Fill 0 22 2 6 1 $woodD

# Tail: thick, low, with a hooked tip.
Line 0 16 38 9 41 $furD 3; Line 0 9 41 4 36 $furD 3; Line 0 4 36 4 30 $fur 3; Line 0 4 30 7 27 $furL 2
# Far legs (behind): dog-leg hind leg, straight foreleg.
Ell 0 20 41 4 5 $furD; Line 0 20 45 16 51 $furD 3; Line 0 16 51 18 58 $furD 3; Fill 0 17 58 5 2 $furD
Line 0 37 41 37 58 $furD 3; Fill 0 36 58 5 2 $furD
# Body: low hips, big hunched shoulders, shaggy.
Ell 0 26 39 12 7 $fur
Ell 0 38 36 10 8 $fur
Ell 0 32 32 11 3 $furL
Ell 0 29 44 10 2 $furD
Grain 0 15 30 34 16 $furD $furH 70 $rng
foreach ($k in 0..14) { $x = 15 + 2 * $k; Line 0 $x 45 ($x + 1) 47 $furD }
# Near legs.
Ell 0 16 40 5 6 $fur; Line 0 15 45 11 51 $fur 3; Line 0 11 51 13 58 $fur 3; Fill 0 11 58 6 2 $furD
Line 0 42 41 43 58 $fur 3; Fill 0 41 58 6 2 $furD; Line 0 41 42 41 52 $furL
foreach ($cx in 11, 14, 18, 21, 41, 44, 35, 38) { Px 0 $cx 60 $claw }

# Three heads on three necks fanning from the shoulders: high, middle (lead), low.
function Head($nx, $ny, $hx, $hy, $open) {
  Line 0 $nx $ny $hx $hy $fur 5
  Line 0 $nx ($ny - 2) $hx ($hy - 2) $furL
  Ell 0 $hx $hy 5 4 $head
  Ell 0 ($hx - 1) ($hy - 2) 3 1 $headL
  Fill 0 ($hx + 3) ($hy - 2) 6 3 $head; Fill 0 ($hx + 3) ($hy - 2) 6 1 $headL
  if ($open) {
    Fill 0 ($hx + 3) ($hy + 1) 6 2 $INK
    Line 0 ($hx + 2) ($hy + 3) ($hx + 8) ($hy + 2) $muzzle 2
    foreach ($t in 4, 6, 8) { Px 0 ($hx + $t) ($hy + 1) $tooth; Px 0 ($hx + $t - 1) ($hy + 2) $tooth }
  } else { Fill 0 ($hx + 3) ($hy + 1) 6 1 $muzzle; Px 0 ($hx + 5) ($hy + 1) $INK }
  Px 0 ($hx + 9) ($hy - 2) $INK
  Line 0 ($hx - 2) ($hy - 3) ($hx - 5) ($hy - 7) $headD 2
  Fill 0 ($hx + 1) ($hy - 2) 2 1 $eye
}
Head 42 32 50 21 $false
Head 44 37 53 32 $true
Head 42 42 49 43 $true
# Shaggy ruff over where the necks join.
foreach ($k in 0..6) { Line 0 (40 + $k) (28 + $k) (43 + $k) (31 + $k) $furD }

# Wires: from the cross-arms down into the fur and to the heads.
Line 0 15 7 13 22 $wire; Line 0 13 22 16 32 $wire
Line 0 35 7 42 15 $wire; Line 0 42 15 46 20 $wire
Line 0 32 13 38 24 $wire; Line 0 38 24 44 30 $wire
# Brows and glowing eyes last, so nothing covers them.
foreach ($h in @(@(50, 21), @(53, 32), @(49, 43))) { Fill 0 ($h[0] - 1) ($h[1] - 4) 6 1 $headD; Fill 0 ($h[0] + 1) ($h[1] - 2) 3 1 $INK; Fill 0 ($h[0] + 1) ($h[1] - 2) 2 1 $eye; Px 0 ($h[0] + 3) ($h[1] - 2) $arc }

Outline 0

# Electricity over the outline: arcs between insulators, down the wires, and crackling round the heads.
function Zig($x0, $y0, $x1, $y1, $n) {
  $px = $x0; $py = $y0
  for ($i = 1; $i -le $n; $i++) {
    $nx = [int]($x0 + ($x1 - $x0) * $i / $n); $ny = [int]($y0 + ($y1 - $y0) * $i / $n)
    if ($i -lt $n) { $ny += $rng.Next(-2, 3); $nx += $rng.Next(-1, 2) }
    Line 0 $px $py $nx $ny $arc; $px = $nx; $py = $ny
  }
}
Zig 16 5 21 5 2; Zig 30 5 36 5 2; Zig 19 11 32 11 4
Zig 36 8 45 19 4; Zig 14 9 12 21 3
Zig 57 27 61 23 2; Zig 51 47 55 51 2
foreach ($p in @(@(16, 4), @(36, 4), @(45, 19), @(61, 22), @(12, 21))) { Px 0 $p[0] $p[1] $arcW }
# Static sparks in the air (it is never quite where it seems to be).
foreach ($p in @(@(9, 16), @(60, 12), @(62, 40), @(6, 48), @(56, 55), @(40, 4))) { Px 0 $p[0] $p[1] $arc }

Shadow 0 30 61 22
Shadow 0 30 62 17

SaveSheet $Out $Preview $Count '#3a3f3a'
