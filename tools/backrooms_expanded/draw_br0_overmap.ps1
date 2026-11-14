param([string]$Out, [string]$Preview)
# Draws the Level 0 overmap sprite sheet, UndeadPeople-style, 32x32 tiles in a row.
# 0-2 backroom chamber variants (yellow rooms from above), 3 border (solid wall),
# 4 stairs bottom, 5 stairs top, 6 rippling-wall room (br0_thin_room). Landmarks are signboards (sprite_common.ps1).
Add-Type -AssemblyName System.Drawing
$T = 32
$Count = 7
$bmp = New-Object System.Drawing.Bitmap ($Count * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
$rng = New-Object System.Random 70

$carpet = Hex '#b8a24e'; $carpetD = Hex '#a48e40'; $carpetL = Hex '#c8b462'
$wall = Hex '#8e7c42'; $wallL = Hex '#a8955a'; $panel = Hex '#fbf8e8'; $panelG = Hex '#d4c47a'

# A wall run: 2 px of wall top with a 1 px dark line under it (the papered face in shadow).
function WallH($tile, $x, $y, $len) { Fill $tile $x $y $len 2 $wall; Fill $tile $x $y $len 1 $wallL; Fill $tile $x ($y + 2) $len 1 $INK }
function WallV($tile, $x, $y, $len) { Fill $tile $x $y 2 $len $wall; Fill $tile $x $y 1 $len $wallL; Fill $tile ($x + 2) $y 1 $len $INK }
function Lamp($tile, $x, $y) { Fill $tile ($x - 1) ($y - 1) 4 3 $panelG; Fill $tile $x $y 2 1 $panel }
function Chamber($tile, $walls) {
  Fill $tile 0 0 $T $T $carpet
  Grain $tile 0 0 $T $T $carpetD $carpetL 70 $rng
  foreach ($w in $walls) { if ($w[0] -eq 'h') { WallH $tile $w[1] $w[2] $w[3] } else { WallV $tile $w[1] $w[2] $w[3] } }
}

# 0-2: the yellow rooms, three layouts of partition walls with doorway gaps, lit here and there
Chamber 0 @(@('h', 0, 9, 12), @('h', 17, 9, 15), @('v', 20, 12, 11), @('h', 4, 22, 13)); Lamp 0 7 4; Lamp 0 25 16; Lamp 0 9 27
Chamber 1 @(@('v', 10, 0, 13), @('v', 10, 18, 14), @('h', 13, 15, 8), @('h', 25, 15, 7), @('v', 22, 3, 9)); Lamp 1 4 8; Lamp 1 17 25; Lamp 1 27 6
Chamber 2 @(@('h', 0, 6, 20), @('v', 6, 9, 9), @('h', 9, 19, 23), @('v', 26, 22, 10), @('v', 16, 8, 6)); Lamp 2 25 3; Lamp 2 12 13; Lamp 2 4 26
$stain = Hex '#8e7a36'
Ell 2 20 28 3 1 $stain; Ell 1 27 26 2 1 $stain

# 3: border: the wall the yellow rooms never get past
Fill 3 0 0 $T $T $wall; Grain 3 0 0 $T $T (Hex '#7a6a36') $wallL 90 $rng
for ($y = 3; $y -lt $T; $y += 8) { Fill 3 0 $y $T 1 (Hex '#6e5e2e') }

# 4-5: stairs, down and up: a stair icon with an arrow, yellow band
function StairSign($tile, $up) {
  Chamber $tile @()
  Sign $tile (Hex '#c8a83e') (Hex '#8a7220')
  $step = Hex '#8a8478'; $stepL = Hex '#b2ac9e'; $arrow = Hex '#2a2a2a'
  # A staircase in profile, rising to the right, then a bold arrow up or down beside it.
  for ($i = 0; $i -lt 4; $i++) { Fill $tile (7 + 3 * $i) (13 - 2 * $i) 3 (1 + 2 * $i) $step; Fill $tile (7 + 3 * $i) (13 - 2 * $i) 3 1 $stepL }
  Line $tile 7 13 18 5 $INK; Fill $tile 7 14 12 1 $INK
  $ar = Hex '#2a6ad2'
  if ($up) { Fill $tile 22 8 2 6 $ar; Fill $tile 20 8 6 1 $ar; Fill $tile 21 7 4 1 $ar; Fill $tile 22 6 2 1 $ar }
  else { Fill $tile 22 5 2 6 $ar; Fill $tile 20 11 6 1 $ar; Fill $tile 21 12 4 1 $ar; Fill $tile 22 13 2 1 $ar }
}
StairSign 4 $false
StairSign 5 $true

# 6: rippling-wall room: yellow wallpaper bending like a curtain, yellow band
Chamber 6 @(); Sign 6 (Hex '#d9c46e') (Hex '#a08a40')
$face = Hex '#d9c46e'; $stripe = Hex '#a08a40'; $shine = Hex '#f4e8a8'
ORect 6 9 5 14 9 $face
for ($x = 10; $x -lt 23; $x += 3) { for ($y = 5; $y -lt 14; $y++) { $off = [int]([Math]::Round(1.2 * [Math]::Sin(($y + $x) / 1.6))); Px 6 ($x + $off) $y $stripe } }
Px 6 13 7 $shine; Px 6 14 8 $shine; Px 6 18 10 $shine; Px 6 19 11 $shine

SaveSheet $Out $Preview $Count
