param([string]$Out, [string]$Preview)
# Draws the UndeadPeople held-weapon overlays, 64x80 tiles like UndeadPeople's own wielded spears
# (sprite_offset -16, -48: the character's 32x32 tile is x 16-47, y 48-79 of each tile).
# 0 overlay_wielded_br_flamero_mado_off   1 overlay_wielded_br_flamero_mado_on
# UndeadPeople holds a spear steeply across the body: butt below the hand, head up past the shoulder, with a
# short gap where the hand covers the shaft. The spear's shape lives in spear_shape.ps1.
Add-Type -AssemblyName System.Drawing
$T = 32; $TW = 64; $TH = 80
$Count = 2
$bmp = New-Object System.Drawing.Bitmap ($Count * $TW), $TH, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
. "$PSScriptRoot\spear_shape.ps1"

# Butt at the bottom of the character's tile, leaning up and to the right like UndeadPeople's spears; light from
# the upper left, so the lit side is flipped.
$held = Layout 20.5 79.5 1 -2.6 60 -Flip -Split 66 -Width 0.5

# Rows the character's hand covers.
$handTop = 65; $handBottom = 68
function HandGap($tile) { for ($y = $handTop; $y -le $handBottom; $y++) { for ($x = 0; $x -lt $TW; $x++) { $bmp.SetPixel($tile * $TW + $x, $y, [System.Drawing.Color]::Transparent) } } }

DrawSpear 0 $held $false 4.0
HandGap 0
OutlineT 0 $INK

DrawSpear 1 $held $true 4.0
HandGap 1
OutlineT 1 $INK
# Flames curl out of the collar (about x 33, y 46) and rise past the blade.
Ribbon 1 @(@(32, 46), @(30, 45), @(28, 43), @(28, 40), @(29, 37)) 1.8 $false
Ribbon 1 @(@(35, 47), @(37, 46), @(39, 44), @(39, 41), @(38, 38)) 1.8 $false
Ribbon 1 @(@(33, 44), @(33, 41), @(34, 38), @(33, 35)) 1.3 $false
Ribbon 1 @(@(31, 48), @(29, 49), @(27, 48)) 1.1 $false
# One thin strand wraps in front of the collar.
Ribbon 1 @(@(31, 48), @(33, 46), @(35, 44)) 0.8 $true
Embers 1 @(@(27, 35), @(40, 36), @(30, 32), @(37, 31), @(25, 41))

# Save, plus a 6x preview with a gap between tiles and the character's tile marked.
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
