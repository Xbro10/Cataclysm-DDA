param([string]$Out, [string]$Preview)
# Draws the Backrooms Expanded boss sprite sheet: 64x64 tiles (2x2 map tiles) in a row, facing right (the game mirrors them).
# Original art in UndeadPeople style; the figure stands on the bottom rows and the tileset offsets it to sit on its tile.
# 0 Redeemer: a huge dark-blue beast with a crown of wire spines that crackle with electricity.
Add-Type -AssemblyName System.Drawing
$T = 64
$Count = 1
$bmp = New-Object System.Drawing.Bitmap ($Count * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
$rng = New-Object System.Random 64

# ---- 0: the Redeemer
$fur = Hex '#22305a'; $furD = Hex '#151d38'; $furL = Hex '#3a5288'; $furH = Hex '#5470a8'
$spine = Hex '#0c1020'; $spineL = Hex '#2a3352'; $bone = Hex '#c9ccd4'; $boneD = Hex '#8e929c'
$arc = Hex '#8ff0ff'; $arcW = Hex '#f4ffff'; $eye = Hex '#c8ffff'; $claw = Hex '#d8d4c8'

# Tail: long and heavy, hanging then hooking up behind.
Line 0 18 36 10 39 $furD 3; Line 0 10 39 5 34 $furD 3; Line 0 5 34 4 27 $fur 3; Line 0 4 27 6 22 $furL 2
# Far legs (darker, behind): a dog-leg hind leg and a straight foreleg.
Ell 0 22 40 4 5 $furD; Line 0 22 44 18 51 $furD 3; Line 0 18 51 20 58 $furD 3; Fill 0 19 58 5 2 $furD
Line 0 37 40 37 58 $furD 3; Fill 0 36 58 5 2 $furD
# Body: lean hips rising to a big hunched shoulder.
Ell 0 28 38 12 6 $fur
Ell 0 40 34 9 8 $fur
Ell 0 36 31 9 3 $furL
Ell 0 30 42 9 2 $furD
Grain 0 18 28 30 16 $furD $furH 55 $rng
# Near legs: thick thigh, bent hock, long foreleg, bone claws.
Ell 0 18 39 5 6 $fur; Line 0 17 44 13 51 $fur 3; Line 0 13 51 15 58 $fur 3; Fill 0 13 58 6 2 $furD
Line 0 43 40 44 58 $fur 3; Fill 0 42 58 6 2 $furD; Line 0 42 41 42 52 $furL
foreach ($cx in 13, 16, 20, 23, 42, 45, 36, 39) { Px 0 $cx 60 $claw }
# Neck and head, low and forward like a hunting dog.
Ell 0 48 31 5 6 $fur
Ell 0 53 28 6 4 $fur
Ell 0 53 26 4 1 $furL
# Long snout, open bone jaw, teeth.
Fill 0 56 26 7 3 $fur; Fill 0 56 26 7 1 $furL
Fill 0 56 29 7 2 $INK
Line 0 54 32 62 31 $bone 2; Line 0 54 33 61 33 $boneD
foreach ($tx in 57, 59, 61) { Px 0 $tx 29 $bone; Px 0 ($tx - 1) 30 $bone }
Px 0 62 26 $INK
# Ears swept back.
Line 0 50 24 47 19 $furD 2; Line 0 52 24 52 19 $fur 2
# Eye: a cold electric glow.
Fill 0 54 26 2 1 $eye; Px 0 56 26 $arc

# Crown of wires: thin black rods from neck and spine, two with cross-bars and pale insulators like a pylon top.
$spines = @(@(26, 33, 23, 21), @(31, 31, 30, 15), @(36, 28, 37, 12), @(41, 27, 43, 16), @(45, 27, 48, 13), @(49, 26, 53, 18))
foreach ($s in $spines) { Line 0 $s[0] $s[1] $s[2] $s[3] $spine }
foreach ($s in $spines) { Line 0 ($s[0] + 1) $s[1] ($s[2] + 1) ($s[3] + 3) $spineL }
Line 0 34 15 40 15 $spine; Line 0 45 16 51 16 $spine
foreach ($p in @(@(34, 14), @(40, 14), @(45, 15), @(51, 15))) { Px 0 $p[0] $p[1] $boneD }
# Shaggy ruff where the wires meet the fur.
foreach ($k in 0..12) { $x = 25 + 2 * $k; $top = 29 - [int]($k / 3); Line 0 $x ($top + 3) ($x + 1) $top $furD }

Outline 0

# Electricity, drawn after the outline so it glows over it: arcs jumping between spine tips and down a leg.
function Zig($x0, $y0, $x1, $y1, $n) {
  $px = $x0; $py = $y0
  for ($i = 1; $i -le $n; $i++) {
    $nx = [int]($x0 + ($x1 - $x0) * $i / $n); $ny = [int]($y0 + ($y1 - $y0) * $i / $n)
    if ($i -lt $n) { $ny += $rng.Next(-2, 3); $nx += $rng.Next(-1, 2) }
    Line 0 $px $py $nx $ny $arc; $px = $nx; $py = $ny
  }
}
Zig 30 15 37 12 3; Zig 37 12 48 13 4; Zig 48 13 53 18 2; Zig 23 21 30 15 3
Zig 45 41 45 56 5
foreach ($p in @(@(37, 11), @(30, 14), @(48, 12), @(45, 49), @(23, 20))) { Px 0 $p[0] $p[1] $arcW }
# Static sparks in the air around it (it is never quite where it seems to be).
foreach ($p in @(@(12, 24), @(57, 20), @(60, 42), @(9, 46), @(52, 52), @(34, 4))) { Px 0 $p[0] $p[1] $arc }

Shadow 0 31 61 20
Shadow 0 31 62 16

SaveSheet $Out $Preview $Count '#3a3f3a'
