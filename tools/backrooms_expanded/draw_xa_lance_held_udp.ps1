param([string]$Out, [string]$Preview)
# Draws the shock lance in your hands for UndeadPeople (Additions): 64x80 tiles like UndeadPeople's own spears
# (sprite_offset -16, -48). 0 overlay_wielded_xa_shock_lance_off, 1-8 overlay_wielded_xa_shock_lance_on (eight
# animation frames). Held steeply across the body with a gap for the hand, like the Flamero-Mado; shape in
# lance_shape.ps1.
Add-Type -AssemblyName System.Drawing
$T = 32; $TW = 64; $TH = 80
$Count = 9
$bmp = New-Object System.Drawing.Bitmap ($Count * $TW), $TH, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
. "$PSScriptRoot\spear_shape.ps1"
. "$PSScriptRoot\lance_shape.ps1"

$held = Layout 20.5 79.5 1 -2.6 60 -Flip -Split 66 -Width 0.5
$handTop = 65; $handBottom = 68
function HandGap($tile) { for ($y = $handTop; $y -le $handBottom; $y++) { for ($x = 0; $x -lt $TW; $x++) { $bmp.SetPixel($tile * $TW + $x, $y, [System.Drawing.Color]::Transparent) } } }

DrawLance 0 $held $false 4.5
HandGap 0
OutlineT 0 $INK
foreach ($fr in 0..7) {
  DrawLance (1 + $fr) $held $true 4.5
  HandGap (1 + $fr)
  OutlineT (1 + $fr) $INK
  # Bigger, brighter arcs: the held sprite is small on screen.
  LanceArcs (1 + $fr) $held (500 + 43 * $fr) 1.6
}

$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
if ($Preview) {
  $sc = 6
  $pv = New-Object System.Drawing.Bitmap ($bmp.Width * $sc + ($Count - 1) * $sc), ($TH * $sc)
  $g = [System.Drawing.Graphics]::FromImage($pv)
  $g.Clear((Hex '#5c5650'))
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
  $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
  for ($ti = 0; $ti -lt $Count; $ti++) {
    $x0 = $ti * ($TW * $sc + $sc)
    $g.FillRectangle((New-Object System.Drawing.SolidBrush (Hex '#6e675f')), ($x0 + 16 * $sc), (48 * $sc), (32 * $sc), (32 * $sc))
    $src = New-Object System.Drawing.Rectangle ($ti * $TW), 0, $TW, $TH
    $dst = New-Object System.Drawing.Rectangle $x0, 0, ($TW * $sc), ($TH * $sc)
    $g.DrawImage($bmp, $dst, $src, [System.Drawing.GraphicsUnit]::Pixel)
  }
  $pv.Save($Preview, [System.Drawing.Imaging.ImageFormat]::Png)
}
