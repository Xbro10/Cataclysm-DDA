param([string]$Out, [string]$Preview)
# Draws the shock lance's sheet (Additions), UndeadPeople-style, 32x32 tiles in a row.
# 0 shock lance, off (xa_shock_lance_off)
# 1 shock lance, on (xa_shock_lance_on), first animation frame
# 2 shock lance schematic (xa_shock_lance_schematic)
# 3-9 the switched-on lance's other animation frames (1, 3, 4 ... 9 loop; the tileset marks it "animated")
# Same layout as the Flamero-Mado (butt top-right, point down-left); the shape is in lance_shape.ps1.
Add-Type -AssemblyName System.Drawing
$T = 32; $TW = 32; $TH = 32
$Count = 10
$Scratch = $Count
$bmp = New-Object System.Drawing.Bitmap (($Count + 1) * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
. "$PSScriptRoot\spear_shape.ps1"
. "$PSScriptRoot\lance_shape.ps1"
$rng = New-Object System.Random 2525

$ground = Layout 30.5 1.5 -1 1 41 -Flip -Split 66

# ---- 0: off.
DrawLance 0 $ground $false
OutlineT 0 $INK
Shadow 0 15 30 9

# ---- 1 (and 3-9): on, eight frames of crackling arcs.
function DrawOn($tile, $frame) {
  DrawLance $tile $ground $true
  OutlineT $tile $INK
  LanceArcs $tile $ground (900 + 37 * $frame)
  Shadow $tile 15 30 9
}
DrawOn 1 0; foreach ($fr in 1..7) { DrawOn (2 + $fr) $fr }

# ---- 2: schematic: a wiring diagram in a static bag: grey-silver sleeve, dark board, the lance in cyan lines, a red stamp.
$bag = Hex '#9aa0a8'; $bagL = Hex '#c8ccd2'; $board = Hex '#1e2a36'; $boardL = Hex '#2c3c4c'; $line = Hex '#8ee8ff'; $fill = Hex '#2f6f8f'; $stamp = Hex '#d23a2a'
Fill 2 5 7 23 17 $board
Grain 2 5 7 23 17 $boardL $boardL 14 $rng
for ($x = 7; $x -lt 28; $x += 3) { Px 2 $x 8 $boardL; Px 2 $x 22 $boardL }
DrawLance $Scratch (Layout 7.5 21.5 1 -0.62 22) $false
for ($y = 0; $y -lt $T; $y++) { for ($x = 0; $x -lt $T; $x++) {
  if ((SGet $Scratch $x $y).A -eq 0) { continue }
  $edge = $false
  foreach ($d in @(@(1, 0), @(-1, 0), @(0, 1), @(0, -1))) { $nx = $x + $d[0]; $ny = $y + $d[1]; if ($nx -lt 0 -or $ny -lt 0 -or $nx -ge $T -or $ny -ge $T -or (SGet $Scratch $nx $ny).A -eq 0) { $edge = $true } }
  SPix 2 $x $y ($(if ($edge) { $line } else { $fill }))
} }
# A little circuit trace and the red PROTOTYPE stamp.
Line 2 8 10 13 10 $line; Line 2 13 10 13 13 $line; Px 2 13 13 (Hex '#ffffff')
Box 2 19 18 7 4 $stamp; Fill 2 20 19 5 1 $stamp
Box 2 4 6 25 19 $bag
Px 2 5 7 $bagL; Px 2 6 7 $bagL; Px 2 5 8 $bagL; Px 2 27 23 $bagL
Outline 2; Shadow 2 16 26 11

$full = $bmp
$bmp = $full.Clone((New-Object System.Drawing.Rectangle 0, 0, ($Count * $T), $T), $full.PixelFormat)
SaveSheet $Out $Preview $Count '#5c5650'
