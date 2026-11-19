param([string]$Out, [string]$Preview)
# Draws the UndeadPeople held-weapon overlays, 64x80 tiles like UndeadPeople's own wielded spears
# (sprite_offset -16, -48: the character's 32x32 tile is x 16-47, y 48-79 of each tile).
# 0 overlay_wielded_br_flamero_mado_off   1-8 overlay_wielded_br_flamero_mado_on (eight animation frames)
# UndeadPeople holds a spear steeply across the body: butt below the hand, head up past the shoulder, with a
# short gap where the hand covers the shaft. The spear's shape lives in spear_shape.ps1.
Add-Type -AssemblyName System.Drawing
$T = 32; $TW = 64; $TH = 80
$Count = 9
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

# Lit, eight animation frames on tiles 1-8 (see FlameFrame in spear_shape.ps1).
# Flames: big, bright tongues curl out of the collar (about x 33, y 46) on both sides and climb the blade, with
# a white-hot core; the held sprite is small on screen, so they need to be bolder than on the ground sprite.
$script:flameLayers = $FlameBright
function DrawLit($tile, $frame) {
  $script:FlameFrame = $frame; FlameFrameStart
  DrawSpear $tile $held $true 4.0
  HandGap $tile
  OutlineT $tile $INK
  Ribbon $tile @(@(32, 47), @(29, 46), @(27, 43), @(26, 39), @(27, 35), @(26, 31), @(27, 28)) 3.0 $false
  Ribbon $tile @(@(35, 48), @(38, 47), @(40, 44), @(41, 40), @(40, 36), @(41, 32), @(40, 29)) 3.0 $false
  Ribbon $tile @(@(33, 44), @(32, 40), @(34, 36), @(33, 32), @(35, 28), @(34, 25)) 2.2 $false
  Ribbon $tile @(@(37, 45), @(39, 41), @(38, 37), @(40, 33), @(39, 29), @(41, 26)) 2.0 $false
  Ribbon $tile @(@(31, 49), @(28, 50), @(25, 49), @(24, 46)) 1.6 $false
  Ribbon $tile @(@(36, 49), @(39, 50), @(42, 48)) 1.4 $false
  # Strands wrap in front of the collar and the blade's base.
  Ribbon $tile @(@(31, 49), @(33, 46), @(36, 43), @(37, 40)) 1.2 $true
  Ribbon $tile @(@(35, 47), @(34, 44), @(36, 41)) 0.9 $true
  Embers $tile @(@(24, 33), @(43, 30), @(29, 25), @(38, 22), @(23, 40), @(45, 38), @(32, 21), @(26, 28), @(44, 25))
}
foreach ($fr in 0..7) { DrawLit (1 + $fr) $fr }
$script:FlameFrame = 0

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
