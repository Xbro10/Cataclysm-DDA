param([string]$Out, [string]$Preview)
# Draws the Backrooms Expanded weapons sheet, UndeadPeople-style, 32x32 tiles in a row.
# 0 Flamero-Mado spear, unlit (br_flamero_mado_off)
# 1 Flamero-Mado spear, lit   (br_flamero_mado_on)
# 2 Flamero-Mado schematic    (br_flamero_schematic)
# 3, 4, 5 the lit spear's other animation frames (1, 3, 4, 5 loop; the tileset marks it "animated")
# The spear lies corner to corner, butt top-right, blade pointing down-left. Its shape lives in spear_shape.ps1.
Add-Type -AssemblyName System.Drawing
$T = 32; $TW = 32; $TH = 32
$Count = 6
$Scratch = $Count
# One extra tile on the right is scratch space for the schematic's silhouette; it is cropped off before saving.
$bmp = New-Object System.Drawing.Bitmap (($Count + 1) * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
. "$PSScriptRoot\spear_shape.ps1"
$rng = New-Object System.Random 1717

$ground = Layout 30.5 1.5 -1 1 41 -Flip -Split 66

# ---- 0: unlit.
DrawSpear 0 $ground $false
OutlineT 0 $INK
Shadow 0 15 30 9

# ---- 1 (and 3, 4, 5): lit: ribbons of flame curl out of the collar jets, wrap the blade's base and rise.
# Four frames of the same flames, swaying (see FlameFrame in spear_shape.ps1).
function DrawLit($tile, $frame) {
  $script:FlameFrame = $frame; FlameFrameStart
  DrawSpear $tile $ground $true
  OutlineT $tile $INK
  Ribbon $tile @(@(12, 18), @(9, 17), @(7, 15), @(7, 12), @(9, 10), @(10, 8)) 2.5 $false
  Ribbon $tile @(@(15, 20), @(18, 21), @(20, 19), @(21, 16), @(20, 13), @(21, 11)) 2.4 $false
  Ribbon $tile @(@(14, 17), @(15, 14), @(14, 11), @(15, 9)) 1.7 $false
  Ribbon $tile @(@(10, 21), @(7, 21), @(5, 19), @(5, 16)) 1.6 $false
  Ribbon $tile @(@(14, 22), @(16, 24), @(18, 24), @(19, 22)) 1.3 $false
  # One thin strand wraps in front of the collar.
  Ribbon $tile @(@(11, 21), @(13, 19), @(15, 17), @(16, 16)) 1.0 $true
  Embers $tile @(@(11, 6), @(6, 9), @(22, 9), @(17, 7), @(3, 14), @(23, 14))
  Shadow $tile 15 30 9
}
DrawLit 1 0; DrawLit 3 1; DrawLit 4 2; DrawLit 5 3
$script:FlameFrame = 0

# ---- 2: schematic: blueprint in a plastic sleeve, the spear drawn as a pale technical outline, red notes.
$bp = Hex '#2e5a9a'; $bpD = Hex '#21447a'; $bpL = Hex '#4a78b8'; $line = Hex '#e8f0fa'; $fill = Hex '#7fa6d8'; $red = Hex '#d23a2a'; $sleeve = Hex '#c8dae8'
Fill 2 5 7 23 17 $bp
Grain 2 5 7 23 17 $bpD $bpL 18 $rng
for ($x = 7; $x -lt 28; $x += 4) { Fill 2 $x 7 1 17 $bpL }
for ($y = 9; $y -lt 24; $y += 4) { Fill 2 5 $y 23 1 $bpL }
# Draw the spear small on the scratch tile, then copy it over as an outline drawing.
DrawSpear $Scratch (Layout 7.5 21.5 1 -0.62 22) $false
for ($y = 0; $y -lt $T; $y++) { for ($x = 0; $x -lt $T; $x++) {
  if ((SGet $Scratch $x $y).A -eq 0) { continue }
  $edge = $false
  foreach ($d in @(@(1, 0), @(-1, 0), @(0, 1), @(0, -1))) { $nx = $x + $d[0]; $ny = $y + $d[1]; if ($nx -lt 0 -or $ny -lt 0 -or $nx -ge $T -or $ny -ge $T -or (SGet $Scratch $nx $ny).A -eq 0) { $edge = $true } }
  SPix 2 $x $y ($(if ($edge) { $line } else { $fill }))
} }
# Dimension line under it, a callout to the collar, and the red margin note.
Line 2 8 22 25 22 $line; Px 2 8 21 $line; Px 2 25 21 $line
Line 2 17 13 22 9 $line; Fill 2 22 9 3 1 $line
Fill 2 7 9 6 1 $red; Fill 2 7 11 4 1 $red
Box 2 4 6 25 19 $sleeve
Px 2 5 7 (Hex '#ffffff'); Px 2 6 7 (Hex '#ffffff'); Px 2 5 8 (Hex '#ffffff')
Px 2 27 23 $bpD; Px 2 26 23 $bpD; Px 2 27 22 $bpD
Outline 2; Shadow 2 16 26 11

# Crop off the scratch tile and save.
$full = $bmp
$bmp = $full.Clone((New-Object System.Drawing.Rectangle 0, 0, ($Count * $T), $T), $full.PixelFormat)
SaveSheet $Out $Preview $Count '#5c5650'
