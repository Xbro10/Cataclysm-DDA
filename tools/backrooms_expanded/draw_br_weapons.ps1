param([string]$Out, [string]$Preview)
# Draws the Backrooms Expanded weapons sheet, UndeadPeople-style, 32x32 tiles in a row.
# 0 Flamero-Mado spear, unlit (br_flamero_mado_off)
# 1 Flamero-Mado spear, lit   (br_flamero_mado_on)
# 2 Flamero-Mado schematic    (br_flamero_schematic)
# The spear lies corner to corner (butt bottom-left, head top-right) like other long weapons on the ground.
Add-Type -AssemblyName System.Drawing
$T = 32
$Count = 3
$bmp = New-Object System.Drawing.Bitmap ($Count * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
$rng = New-Object System.Random 1717

$wood = Hex '#8a5a32'; $woodL = Hex '#b07a46'; $woodD = Hex '#5e3c20'
$tank = Hex '#4a4e54'; $tankL = Hex '#7c828a'; $tankD = Hex '#2e3034'
$brass = Hex '#c89a3a'; $brassL = Hex '#f0c868'
$blade = Hex '#3a3c40'; $bladeL = Hex '#8e939a'; $bladeD = Hex '#232427'
$heat = Hex '#b8402a'

# The spear itself, shared by both versions. $lit tints the blade's edge red-hot.
function Spear($tile, $lit) {
  # Shaft: two pixels wide, light along the top edge.
  for ($k = 0; $k -le 17; $k++) { $x = 3 + $k; $y = 28 - $k; Px $tile $x $y $wood; Px $tile ($x + 1) $y $woodD; Px $tile $x ($y - 1) $woodL }
  # Butt cap.
  Px $tile 3 28 $tankD; Px $tile 4 28 $tankD; Px $tile 3 27 $tankL
  # Fuel tank sleeve in the middle of the shaft: three pixels wide, steel with a highlight.
  for ($k = 6; $k -le 11; $k++) { $x = 3 + $k; $y = 28 - $k; Px $tile ($x - 1) $y $tank; Px $tile $x $y $tank; Px $tile ($x + 1) $y $tankD; Px $tile $x ($y - 1) $tankL; Px $tile ($x - 1) ($y - 1) $tank }
  # Brass valve knob on the side of the tank, and a thin brass feed line up to the head.
  Fill $tile 13 21 2 2 $brass; Px $tile 13 21 $brassL
  for ($k = 12; $k -le 17; $k++) { Px $tile (4 + $k) (28 - $k) $brass }
  # Jet collar behind the head: a ring of dark steel with brass nozzles.
  Fill $tile 20 9 3 3 $tank; Px $tile 20 9 $tankL; Px $tile 22 11 $tankD; Px $tile 21 8 $brass; Px $tile 23 10 $brass
  # Blade: a blackened leaf with two fins, tip in the top-right corner.
  $w = @(2, 2, 2, 2, 1, 1, 1, 0)
  for ($k = 0; $k -lt 8; $k++) {
    $cx = 22 + $k; $cy = 8 - $k
    for ($j = -$w[$k]; $j -le $w[$k]; $j++) { Px $tile ($cx + $j) ($cy + $j) $blade; Px $tile ($cx + $j + 1) ($cy + $j) $blade }
    Px $tile $cx $cy $bladeL
    if ($lit) { Px $tile ($cx + $w[$k] + 1) ($cy + $w[$k]) $heat } else { Px $tile ($cx - $w[$k]) ($cy - $w[$k]) $bladeD }
  }
  Px $tile 29 1 $bladeL
  # Fins swept back from the collar.
  Px $tile 19 7 $bladeD; Px $tile 18 6 $blade; Px $tile 24 12 $bladeD; Px $tile 25 13 $blade
}

# ---- 0: unlit.
Spear 0 $false
Outline 0; Shadow 0 15 30 9

# ---- 1: lit: the same spear with flame streaming off the jets and around the head.
Spear 1 $true
$f1 = Hex '#ffe27a'; $f2 = Hex '#ffa83a'; $f3 = Hex '#e8582a'; $f4 = Hex '#a8301e'
# Flame as a halo around the head: rings of fire grow out from the blade and collar, hottest (yellow) next to the
# steel, red at the edge, and taller above the head than below it because fire rises.
$head = New-Object System.Collections.Generic.List[object]
for ($y = 0; $y -lt 15; $y++) { for ($x = 16; $x -lt $T; $x++) { if ($bmp.GetPixel(1 * $T + $x, $y).A -ne 0 -and ($x + $y) -ge 26 -and ($x - $y) -ge 9) { $head.Add(@($x, $y)) } } }
$paint = @{}
for ($y = 0; $y -lt 18; $y++) { for ($x = 13; $x -lt $T; $x++) {
  if ($bmp.GetPixel(1 * $T + $x, $y).A -ne 0) { continue }
  $best = 99.0
  foreach ($p in $head) {
    $dx = $x - $p[0]; $dy = $y - $p[1]
    # Below the head counts double, above it counts half: the halo leans upward.
    $dy2 = if ($dy -gt 0) { $dy * 2.0 } else { $dy * 0.55 }
    $d = [Math]::Sqrt($dx * $dx + $dy2 * $dy2)
    if ($d -lt $best) { $best = $d }
  }
  $jit = $rng.NextDouble() * 1.1
  $v = $best + $jit
  if ($v -le 1.5) { $paint["$x,$y"] = $f1 } elseif ($v -le 2.5) { $paint["$x,$y"] = $f2 } elseif ($v -le 3.4) { $paint["$x,$y"] = $f3 } elseif ($v -le 4.0 -and $rng.NextDouble() -lt 0.6) { $paint["$x,$y"] = $f4 }
} }
foreach ($k in $paint.Keys) { $xy = $k.Split(','); Px 1 ([int]$xy[0]) ([int]$xy[1]) $paint[$k] }
# Hot core right at the jets, over the collar.
Px 1 21 8 $f1; Px 1 23 9 $f1; Px 1 22 7 $f2
Outline 1
Shadow 1 15 30 9

# ---- 2: schematic: a blueprint sheet in a plastic sleeve, the spear's cutaway drawn in white, a red note.
$bp = Hex '#2e5a9a'; $bpD = Hex '#21447a'; $bpL = Hex '#4a78b8'; $ink = Hex '#e8f0fa'; $red = Hex '#d23a2a'; $sleeve = Hex '#c8dae8'
Fill 2 6 7 21 17 $bp
Grain 2 6 7 21 17 $bpD $bpL 18 $rng
# Grid lines.
for ($x = 8; $x -lt 27; $x += 4) { Fill 2 $x 7 1 17 $bpL }
for ($y = 9; $y -lt 24; $y += 4) { Fill 2 6 $y 21 1 $bpL }
# The spear in cutaway: shaft, tank outline and the head.
Line 2 8 20 24 10 $ink
Box 2 13 15 5 3 $ink
Px 2 25 9 $ink; Px 2 24 9 $ink; Px 2 25 10 $ink; Px 2 23 9 $ink
# Dimension ticks and the red margin note.
Px 2 9 22 $ink; Px 2 23 22 $ink; Line 2 9 22 23 22 $ink
Fill 2 8 9 6 1 $red; Fill 2 8 11 4 1 $red
# Plastic sleeve: a pale border with a glint, and a folded corner.
Box 2 5 6 23 19 $sleeve
Px 2 6 7 (Hex '#ffffff'); Px 2 7 7 (Hex '#ffffff'); Px 2 6 8 (Hex '#ffffff')
Px 2 26 23 $bpD; Px 2 25 23 $bpD; Px 2 26 22 $bpD
Outline 2; Shadow 2 16 26 10

SaveSheet $Out $Preview $Count '#5c5650'
