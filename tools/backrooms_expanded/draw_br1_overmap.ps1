param([string]$Out, [string]$Preview)
# Draws the Level 1 overmap sprite sheet: ten 32x32 tiles in a row, UndeadPeople-style.
# 0-2 garage variants, 3 arrival alcove, 4 Breakroom, 5 squatter camp, 6 dead zone, 7 overrun camp, 8 elevator lobby,
# 9 generator room (marked by an oil-stained note).
# Plain squares are textured ground; landmarks are signboards with a pictogram (see sprite_common.ps1).
Add-Type -AssemblyName System.Drawing
$T = 32
$Count = 10
$bmp = New-Object System.Drawing.Bitmap ($Count * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
$rng = New-Object System.Random 1337

$con = Hex '#4e5054'; $conD = Hex '#3e4044'; $conL = Hex '#5c5e62'
$line = Hex '#b89a2e'; $lineD = Hex '#7e6a20'
$pil = Hex '#6a6c70'; $pilL = Hex '#86888c'
$lit = Hex '#f2ecc0'; $glow = Hex '#7a7a68'; $dead = Hex '#2e3034'

function Garage($tile, $dark = $false) {
  $base = if ($dark) { Hex '#363839' } else { $con }
  Fill $tile 0 0 $T $T $base
  Grain $tile 0 0 $T $T $conD $conL 90 $rng
  $ln = if ($dark) { $lineD } else { $line }
  foreach ($x in 0, 4, 8, 12, 16, 20, 24, 28) { Fill $tile $x 1 1 7 $ln; Fill $tile $x 17 1 7 $ln }
  foreach ($x in 4, 12, 20, 28) { ORect $tile ($x - 1) 3 3 3 $pil; Px $tile ($x - 1) 3 $pilL; ORect $tile ($x - 1) 19 3 3 $pil; Px $tile ($x - 1) 19 $pilL }
}
function Light($tile, $x, $y, $on) {
  if ($on) { Fill $tile ($x - 2) ($y - 1) 5 3 $glow; ORect $tile ($x - 1) $y 3 1 $lit } else { ORect $tile ($x - 1) $y 3 1 $dead }
}
function Car($tile, $x, $y, $body, $roof) {
  ORect $tile $x $y 4 7 $body
  Fill $tile $x ($y + 2) 4 3 $roof
  Fill $tile ($x + 1) ($y + 1) 2 1 (Hex '#a8c4d4'); Fill $tile ($x + 1) ($y + 5) 2 1 (Hex '#6f8794')
}

# 0-2: the garage, plain / with cars / dim and wet
Garage 0; Light 0 8 11 $true; Light 0 24 11 $true; Light 0 16 28 $true
Garage 1; Light 1 8 11 $true; Light 1 24 11 $false; Light 1 16 28 $true
Car 1 1 1 (Hex '#8a3b34') (Hex '#a85a50'); Car 1 21 17 (Hex '#3a4f6e') (Hex '#5a739a'); Car 1 9 17 (Hex '#6b6b5a') (Hex '#8e8e78')
Garage 2; Light 2 8 11 $false; Light 2 24 11 $false; Light 2 16 28 $true
$pud = Hex '#3f5868'; $pudL = Hex '#6f8fa2'
ORect 2 10 12 5 2 $pud; Fill 2 11 12 2 1 $pudL; ORect 2 22 26 4 2 $pud; Fill 2 23 26 2 1 $pudL

# 3: arrival alcove: a rippling cyan wall on the sign
Garage 3; Sign 3 (Hex '#3fa3b5') (Hex '#2a7080')
$c1 = Hex '#7fd6e0'; $c2 = Hex '#3fa3b5'; $pipe = Hex '#4a5a4c'
ORect 3 9 5 14 9 $pipe
for ($x = 10; $x -lt 22; $x++) { for ($y = 6; $y -lt 13; $y++) { if ((($x + $y) % 3) -eq 0) { Px 3 $x $y $c1 } else { Px 3 $x $y $c2 } } }

# 4: the Breakroom: a steaming mug, green band
Garage 4; Sign 4 (Hex '#4f9a4a') (Hex '#2f6a2c')
$mug = Hex '#eeeae0'; $mugD = Hex '#b8b4aa'; $tea = Hex '#7a4a26'; $steam = Hex '#f4f2ec'
ORect 4 11 8 9 6 $mug; Fill 4 18 9 1 5 $mugD; Fill 4 12 8 7 1 $tea
Box 4 20 9 3 4 $INK; Px 4 21 10 $mug; Px 4 21 11 $mug
Px 4 13 6 $steam; Px 4 14 5 $steam; Px 4 16 6 $steam; Px 4 17 5 $steam

# 5: squatter camp: sandbags around a fire, tan band
Garage 5; Sign 5 (Hex '#a8915e') (Hex '#73633e')
$sand = Hex '#c8ae76'; $sandD = Hex '#94804e'; $fire = Hex '#ff8c1a'; $fireY = Hex '#ffd96a'
foreach ($p in @(@(9,12), @(14,12), @(19,12), @(11,10), @(17,10))) { OEll 5 $p[0] $p[1] 2 1 $sand; Px 5 ($p[0] + 1) ($p[1] + 1) $sandD }
OEll 5 14 7 2 2 $fire; Px 5 14 7 $fireY; Px 5 14 5 $fire

# 6: dead zone: the garage with every light out, darker, more cars
Garage 6 $true; Light 6 8 11 $false; Light 6 24 11 $false; Light 6 16 28 $false
Car 6 1 1 (Hex '#4a2c29') (Hex '#5e3a36'); Car 6 13 17 (Hex '#2a3546') (Hex '#3a4a60'); Car 6 25 1 (Hex '#46463c') (Hex '#57574a')

# 7: overrun camp: knocked-over sandbags and blood, dark red band
Garage 7; Sign 7 (Hex '#7a2a24') (Hex '#4e1814')
$blood = Hex '#8a1e1a'; $bloodD = Hex '#5a1210'; $ash = Hex '#5a5450'
foreach ($p in @(@(9,12), @(20,11), @(12,8))) { OEll 7 $p[0] $p[1] 2 1 $sand }
OEll 7 16 11 2 1 $ash
Fill 7 14 6 3 2 $blood; Px 7 17 7 $blood; Px 7 13 9 $bloodD; Fill 7 18 12 2 1 $bloodD; Px 7 22 8 $blood

# 8: elevator lobby: two steel doors with up/down arrows, light cyan band
Garage 8; Sign 8 (Hex '#5fb8d0') (Hex '#3a8094')
$steel = Hex '#d4d8dc'; $steelD = Hex '#8e959c'; $arrow = Hex '#ff9a3a'
ORect 8 9 6 6 9 $steel; ORect 8 17 6 6 9 $steel; Fill 8 14 6 1 9 $steelD; Fill 8 22 6 1 9 $steelD
Px 8 15 5 $arrow; Px 8 16 5 $arrow; Px 8 15 4 $arrow; Px 8 16 4 $arrow


# 9: generator room: a squat generator with a lightning bolt, amber band
Garage 9; Sign 9 (Hex '#e0a82e') (Hex '#9a7018')
$gen = Hex '#c8432e'; $genD = Hex '#8a2c1e'; $bolt = Hex '#ffe24a'
ORect 9 7 7 11 7 $gen; Fill 9 8 12 9 1 $genD; Fill 9 9 9 3 2 (Hex '#3a3a3a'); Px 9 10 6 $INK; Px 9 15 6 $INK
Line 9 22 4 20 9 $bolt; Line 9 20 9 23 9 $bolt; Line 9 23 9 21 14 $bolt

SaveSheet $Out $Preview $Count
