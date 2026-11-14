param([string]$Out, [string]$Preview)
# Draws the Level 2 overmap sprite sheet: seven 32x32 tiles in a row, UndeadPeople-style.
# 0-1 tunnel variants, 2 service ladder, 3 boiler room, 4 valve junction, 5 flooded dip, 6 Tunnel Crew camp.
# Plain tunnels are textured ground; landmarks are signboards with a pictogram (see sprite_common.ps1).
# One map square is 24 tiles; at 32 px the corridor lines (tiles 3-5, 11-13, 19-21) sit at px 4-7, 14-17, 25-28.
Add-Type -AssemblyName System.Drawing
$T = 32
$Count = 7
$bmp = New-Object System.Drawing.Bitmap ($Count * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
$rng = New-Object System.Random 2002

$wall = Hex '#2c2824'; $wallD = Hex '#221f1c'; $wallL = Hex '#3a3530'
$floor = Hex '#6a6762'; $floorD = Hex '#58554f'; $floorL = Hex '#7a7772'
$copper = Hex '#b0703c'; $copperD = Hex '#7a4a26'; $iron = Hex '#4f5559'
$bulb = Hex '#f0b84a'; $bulbGlow = Hex '#6e5a3c'

function Walls($tile) { Fill $tile 0 0 $T $T $wall; Grain $tile 0 0 $T $T $wallD $wallL 80 $rng }
# A corridor piece ($edgeOnly: draw only its outline; draw every outline before any floor so crossings stay open), with an outline and pipe runs along both walls.
function Corr($tile, $h, $b, $a0, $a1, $edgeOnly = $false) {
  if ($edgeOnly) { if ($h) { Fill $tile $a0 ($b - 1) ($a1 - $a0) 6 $INK } else { Fill $tile ($b - 1) $a0 6 ($a1 - $a0) $INK }; return }
  if ($h) {
    Fill $tile $a0 $b ($a1 - $a0) 4 $floor
    Grain $tile $a0 $b ($a1 - $a0) 4 $floorD $floorL 8 $rng
    Fill $tile $a0 $b ($a1 - $a0) 1 $copper; Fill $tile $a0 ($b + 3) ($a1 - $a0) 1 $iron
    for ($i = $a0; $i -lt $a1; $i += 4) { Px $tile $i $b $copperD }
  } else {
    Fill $tile $b $a0 4 ($a1 - $a0) $floor
    Grain $tile $b $a0 4 ($a1 - $a0) $floorD $floorL 8 $rng
    Fill $tile $b $a0 1 ($a1 - $a0) $copper; Fill $tile ($b + 3) $a0 1 ($a1 - $a0) $iron
    for ($j = $a0; $j -lt $a1; $j += 4) { Px $tile $b $j $copperD }
  }
}
# Draws a whole corridor network: each entry is @($h, $b, $a0, $a1); the centre cross is always included.
function Net($tile, $pieces) {
  $all = @(@($true, 14, 0, $T), @($false, 14, 0, $T)) + $pieces
  foreach ($p in $all) { Corr $tile $p[0] $p[1] $p[2] $p[3] $true }
  foreach ($p in $all) { Corr $tile $p[0] $p[1] $p[2] $p[3] }
}
function Cross($tile) { Net $tile @() }
function Bulb($tile, $x, $y) { Fill $tile ($x - 1) ($y - 1) 3 3 $bulbGlow; Px $tile $x $y $bulb }

# 0-1: tunnels with different side corridors
Walls 0; Net 0 @(@($true, 4, 0, 14), @($false, 25, 14, 32), @($true, 25, 18, 32)); Bulb 0 15 15
Walls 1; Net 1 @(@($false, 4, 0, 18), @($true, 25, 0, 14), @($true, 4, 18, 30)); Bulb 1 26 15

# 2: service ladder: rungs climbing to a hatch, grey-blue band
Walls 2; Cross 2; Sign 2 (Hex '#6a7c8e') (Hex '#47566a')
$rail = Hex '#9aa3ab'; $rung = Hex '#d6dbe0'
ORect 2 12 4 1 11 $rail; ORect 2 19 4 1 11 $rail
foreach ($y in 6, 9, 12) { Fill 2 13 $y 6 1 $rung }
ORect 2 11 3 10 1 (Hex '#5a6470')

# 3: boiler room: a red-hot drum with flames, red band
Walls 3; Cross 3; Sign 3 (Hex '#9a3022') (Hex '#651c14')
$drum = Hex '#8a2a1e'; $drumL = Hex '#b8442e'; $flame = Hex '#ff8c1a'; $flameY = Hex '#ffd96a'
ORect 3 10 7 12 7 $drum; Fill 3 10 7 12 1 $drumL; Fill 3 13 9 1 4 (Hex '#5a1a12'); Fill 3 18 9 1 4 (Hex '#5a1a12')
OEll 3 16 5 2 1 $flame; Px 3 16 5 $flameY; Px 3 13 6 $flame; Px 3 19 6 $flame

# 4: valve junction: a red valve wheel with spokes, red band
Walls 4; Cross 4; Sign 4 (Hex '#b03a2a') (Hex '#7a241a')
$wheel = Hex '#d23a2a'; $wheelL = Hex '#f06a4a'; $hub = Hex '#3a3a3a'
OEll 4 16 9 5 4 $wheel; Ell 4 16 9 3 2 (Hex '#b2aea4')
Line 4 13 9 19 9 $wheel; Line 4 16 6 16 12 $wheel
Px 4 14 6 $wheelL; Px 4 13 7 $wheelL; Fill 4 15 8 3 3 $hub

# 5: flooded dip: a big water drop, blue band
Walls 5; Cross 5; Sign 5 (Hex '#3f6f9a') (Hex '#2a4c6a')
$water = Hex '#4a8ac0'; $waterL = Hex '#8ac4ea'
OEll 5 16 10 4 4 $water; Line 5 16 4 14 7 $INK; Line 5 16 4 18 7 $INK; Fill 5 15 5 3 3 $water; Px 5 16 4 $water
Px 5 14 9 $waterL; Px 5 14 10 $waterL; Px 5 15 8 $waterL

# 6: Tunnel Crew camp: a wrench over a hard hat, orange band
Walls 6; Cross 6; Sign 6 (Hex '#c47a2a') (Hex '#8a521a')
$hat = Hex '#f0c030'; $hatD = Hex '#b88a1a'; $steel = Hex '#c8ccd0'
OEll 6 16 11 5 2 $hat; Fill 6 11 12 11 1 $hatD; Fill 6 10 13 13 1 $hatD
Line 6 10 5 21 9 $INK 2; Line 6 11 5 21 8 $steel; Fill 6 9 4 3 3 $steel; Px 6 10 5 (Hex '#b2aea4')

SaveSheet $Out $Preview $Count
