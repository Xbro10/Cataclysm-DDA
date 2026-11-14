param([string]$Out, [string]$Preview)
# Draws the Level 0 terrain sheet (walls and floors), UndeadPeople-style, 32x32 tiles in a row.
#  0-15  yellow wallpaper wall, one sprite per connection mask (bits: S=1, E=2, W=4, N=8, as in cata_tiles)
# 16-31  grey wall, same masks
# 32-34  damp carpet (three stain variants)   35  carpet under a ceiling light
# 36     faded tile                           37  tile under a ceiling light
# A wall shows its papered face only where no wall continues south (the 3/4 top-down look);
# elsewhere you see the top of the wall, with a dark edge on every unconnected side.
Add-Type -AssemblyName System.Drawing
$T = 32
$Count = 38
$bmp = New-Object System.Drawing.Bitmap ($Count * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
$rng = New-Object System.Random 1980

function Wall($tile, $mask, $pal) {
  $S = ($mask -band 1) -ne 0; $E = ($mask -band 2) -ne 0; $W = ($mask -band 4) -ne 0; $N = ($mask -band 8) -ne 0
  # Top of the wall everywhere, with grain.
  Fill $tile 0 0 $T $T $pal.Top
  Grain $tile 0 0 $T $T $pal.TopD $pal.TopL 40 $rng
  if (-not $N) { Fill $tile 0 0 $T 1 $INK; Fill $tile 0 1 $T 1 $pal.TopL }
  if (-not $W) { Fill $tile 0 0 1 $T $INK; Fill $tile 1 1 1 ($T - 1) $pal.TopL }
  if (-not $E) { Fill $tile ($T - 1) 0 1 $T $INK; Fill $tile ($T - 2) 1 1 ($T - 1) $pal.TopD }
  if (-not $S) {
    # The papered face: 12 px tall, vertical stripes, damp stains, a baseboard at the bottom.
    $fy = 18
    Fill $tile 0 ($fy - 1) $T 1 $INK
    Fill $tile 0 $fy $T ($T - $fy) $pal.Face
    for ($x = 1; $x -lt $T; $x += 4) { Fill $tile $x $fy 1 ($T - $fy - 3) $pal.Stripe }
    Grain $tile 0 $fy $T ($T - $fy - 3) $pal.Stripe $pal.FaceL 14 $rng
    if ($rng.Next(0, 3) -eq 0) { $sx = $rng.Next(3, 26); Fill $tile $sx ($fy + 2) 3 4 $pal.Stain; Px $tile ($sx + 1) ($fy + 6) $pal.Stain }
    Fill $tile 0 ($T - 3) $T 2 $pal.Base; Fill $tile 0 ($T - 1) $T 1 $INK
    if (-not $W) { Fill $tile 0 ($fy - 1) 1 ($T - $fy + 1) $INK }
    if (-not $E) { Fill $tile ($T - 1) ($fy - 1) 1 ($T - $fy + 1) $INK }
  }
}

$yellow = @{ Top = (Hex '#8e7c42'); TopD = (Hex '#7a6a36'); TopL = (Hex '#a8955a'); Face = (Hex '#d9c46e'); FaceL = (Hex '#e6d48a'); Stripe = (Hex '#c6b05c'); Stain = (Hex '#a8924a'); Base = (Hex '#6e5a2e') }
$gray = @{ Top = (Hex '#6e6c68'); TopD = (Hex '#5e5c58'); TopL = (Hex '#868480'); Face = (Hex '#a8a6a0'); FaceL = (Hex '#b8b6b0'); Stripe = (Hex '#9a9892'); Stain = (Hex '#86847e'); Base = (Hex '#4a4844') }
for ($m = 0; $m -lt 16; $m++) { Wall $m $m $yellow; Wall (16 + $m) $m $gray }

# Floors: damp yellow carpet (three variants), then a lit version; faded tile and its lit version.
$carpet = Hex '#b8a24e'; $carpetD = Hex '#a48e40'; $carpetL = Hex '#c8b462'; $damp = Hex '#8e7a36'
function Carpet($tile, $stains) {
  Fill $tile 0 0 $T $T $carpet
  for ($y = 0; $y -lt $T; $y += 2) { for ($x = ($y % 4) / 2; $x -lt $T; $x += 2) { Px $tile $x $y $carpetD } }
  Grain $tile 0 0 $T $T $carpetD $carpetL 60 $rng
  for ($k = 0; $k -lt $stains; $k++) { $cx = $rng.Next(5, 27); $cy = $rng.Next(5, 27); Ell $tile $cx $cy ($rng.Next(2, 5)) ($rng.Next(1, 3)) $damp }
}
Carpet 32 0; Carpet 33 1; Carpet 34 2
function Panel($tile) {
  $glow = [System.Drawing.Color]::FromArgb(255, 236, 226, 170); $lightC = Hex '#fbf8e8'; $frame = Hex '#9a9a92'
  for ($y = 6; $y -lt 26; $y++) { for ($x = 6; $x -lt 26; $x++) {
    $c = $bmp.GetPixel($tile * $T + $x, $y); Px $tile $x $y ([System.Drawing.Color]::FromArgb(255, [Math]::Min(255, $c.R + 28), [Math]::Min(255, $c.G + 26), [Math]::Min(255, $c.B + 18))) } }
  ORect $tile 10 12 12 8 $frame; Fill $tile 11 13 10 6 $lightC; Fill $tile 11 15 10 1 $glow; Fill $tile 11 16 10 1 $glow
}
Carpet 35 0; Panel 35
$tileC = Hex '#c9c4b0'; $tileD = Hex '#b2ad98'; $grout = Hex '#9a9480'
function Tiles($tile) {
  Fill $tile 0 0 $T $T $tileC
  Grain $tile 0 0 $T $T $tileD (Hex '#d6d2c0') 40 $rng
  for ($i = 0; $i -lt $T; $i += 8) { Fill $tile $i 0 1 $T $grout; Fill $tile 0 $i $T 1 $grout }
  Fill $tile 17 9 6 4 (Hex '#b4a878')
}
Tiles 36; Tiles 37; Panel 37

SaveSheet $Out $Preview $Count
